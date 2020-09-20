/***************************************************************************
# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

class histogram
{
private:
    double mMinValue, mMaxValue;
    size_t mValueCount, mErrorValueCount;
    double mBucketSize;
    std::vector<size_t> mvBuckets;
    size_t mBucketIdRange[2];
    size_t mBucketIdMax;

public:
    histogram(size_t buckets, double minValue = 0.0, double maxValue = 1.0)
    {
        this->init(buckets, minValue, maxValue);
    }

    void init(size_t buckets, double minValue, double maxValue, size_t value = 0)
    {
        this->mMinValue = minValue;
        this->mMaxValue = maxValue;
        this->mValueCount = 0;
        this->mErrorValueCount = 0;
        this->mBucketIdRange[0] = std::string::npos;
        this->mBucketIdRange[1] = 0;
        this->resize(buckets);
        this->mvBuckets.resize(buckets, value);
    }

    double getBucketSize() const { return mBucketSize; }
    size_t getBucketIdMin() const { return mBucketIdRange[0]; }
    size_t getBucketIdMax() const { return mBucketIdRange[1]; }
    size_t getBucketValue(size_t bucketId) const { return mvBuckets[bucketId]; }
    size_t size() const { return mvBuckets.size(); }
    double getMinValue() const { return this->mMinValue; }
    double getMaxValue() const { return this->mMaxValue; }
    double getBucketStep() const { return (this->mMaxValue - this->mMinValue) / this->mvBuckets.size(); }

    void clear(size_t value = 0)
    {
        this->mvBuckets.resize(mvBuckets.size(), value);
    }

    void resize(size_t buckets)
    {
        this->mBucketSize = (this->mMaxValue - this->mMinValue) / buckets;
        this->mvBuckets.resize(buckets);
    }

    size_t valueBucketId(double value) const
    {
        if (value < this->mMinValue || value > this->mMaxValue)
            return std::string::npos;

        size_t bucketId = size_t(value / this->mBucketSize);

        if (bucketId == this->mvBuckets.size())
        {
            bucketId--;
        }
        return bucketId;
    }

    void inc(double value, size_t amount = 1)
    {
        size_t bucketId = valueBucketId(value);
        if (bucketId != std::string::npos)
        {
            this->mvBuckets[bucketId] += amount;
            this->mValueCount += amount;
            this->mBucketIdRange[0] = std::min(this->mBucketIdRange[0], bucketId);
            this->mBucketIdRange[1] = std::max(this->mBucketIdRange[1], bucketId);
        }
        else
        {
            mErrorValueCount += amount;
        }
    }

    std::string toPython(size_t numPixels, float ppd, double meanValue, double stddevValue, double maxValue, double minValue, double weightedMedian, double firstWeightedQuartile, double thirdWeightedQuartile, const bool optionLog) const
    {
        std::stringstream ss;

        double bucketStep = getBucketStep();

        //  imports
        ss << "import matplotlib.pyplot as plt\n";
        ss << "import sys\n";
        ss << "import numpy as np\n";
        ss << "from matplotlib.ticker import (MultipleLocator)\n\n";

        ss << "dimensions = (25, 15)  #  centimeters\n\n";

        ss << "lineColor = 'blue'\n";
        ss << "fillColor = 'lightblue'\n";
        ss << "meanLineColor = 'gray'\n";
        ss << "stddevLineColor = 'orange'\n";
        ss << "weightedMedianLineColor = 'red'\n";
        ss << "quartileLineColor = 'purple'\n";
        ss << "cogLineColor = 'green'\n";
        ss << "fontSize = 14\n";
        ss << "numPixels = " << numPixels << "\n\n";

        ss << "ppd = " << ppd << "\n";
        ss << "meanValue = " << meanValue << "\n";
        ss << "stddevValue = " << stddevValue << "\n";
        ss << "maxValue = " << maxValue << "\n";
        ss << "minValue = " << minValue << "\n\n";
        ss << "weightedMedianValue = " << weightedMedian << "\n\n";
        ss << "firstWeightedQuartileValue = " << firstWeightedQuartile << "\n\n";
        ss << "thirdWeightedQuartileValue = " << thirdWeightedQuartile << "\n\n";

        //  X-axis
        ss << "dataX = [";
        for (size_t bucketId = 0; bucketId < this->mvBuckets.size(); bucketId++)
        {
            ss << (bucketId > 0 ? ", " : "");
            ss << bucketStep * bucketId + 0.5 * bucketStep;
        }
        ss << "]\n\n";

        //  FLIP histogram
        ss << "dataFLIP = [";
        for (size_t bucketId = 0; bucketId < this->mvBuckets.size(); bucketId++)
        {
            ss << (bucketId > 0 ? ", " : "");
            ss << this->mvBuckets[bucketId];
        }
        ss << "]\n\n";

        //  Weighted FLIP histogram
        ss << "bucketStep = " << bucketStep << "\n";
        ss << "weightedDataFLIP = np.empty(" << this->mvBuckets.size() << ")\n";
        ss << "moments = np.empty(" << this->mvBuckets.size() << ")\n";
        ss << "for i in range(" << this->mvBuckets.size() << ") :\n";
        ss << "\tweight = (i + 0.5) * bucketStep\n";
        ss << "\tweightedDataFLIP[i] = dataFLIP[i] * weight\n";
        ss << "\tmoments[i] = dataFLIP[i] * weight * weight\n";
        ss << "cog = sum(moments) / sum(weightedDataFLIP)\n";
        ss << "weightedDataFLIP /= (numPixels /(1024 * 1024))  # normalized with the number of megapixels in the image\n\n";
        if (optionLog)
        {
            ss << "for i in range(" << this->mvBuckets.size() << ") :\n";
            ss << "\tif weightedDataFLIP[i] > 0 :\n";
            ss << "\t\tweightedDataFLIP[i] = np.log10(weightedDataFLIP[i])  # avoid log of zero\n\n";
        }

        ss << "maxY = max(weightedDataFLIP)\n\n";

        ss << "sumWeightedDataFLIP = sum(weightedDataFLIP)\n\n";

        ss << "font = { 'family' : 'serif', 'style' : 'normal', 'weight' : 'normal', 'size' : fontSize }\n";
        ss << "lineHeight = fontSize / (dimensions[1] * 15)\n";
        ss << "plt.rc('font', **font)\n";
        ss << "fig = plt.figure()\n";
        ss << "axes = plt.axes()\n";
        ss << "axes.xaxis.set_minor_locator(MultipleLocator(0.1))\n";
        ss << "axes.xaxis.set_major_locator(MultipleLocator(0.2))\n\n";

        ss << "fig.set_size_inches(dimensions[0] / 2.54, dimensions[1] / 2.54)\n"; 

        if(optionLog)
            ss << "axes.set(title = 'Weighted \\uA7FBLIP Histogram', xlabel = '\\uA7FBLIP error', ylabel = 'log(weighted \\uA7FBLIP sum per megapixel)')\n\n";
        else
            ss << "axes.set(title = 'Weighted \\uA7FBLIP Histogram', xlabel = '\\uA7FBLIP error', ylabel = 'Weighted \\uA7FBLIP sum per megapixel')\n\n";

        ss << "plt.bar(dataX, weightedDataFLIP, width = " << bucketStep << ", color = fillColor, edgeColor = lineColor, lineWidth = 0.3)\n\n";

        ss << "plt.text(0.99, 1.0 - 1 * lineHeight, 'PPD: ' + str(f'{ppd:.1f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes, color='black')\n\n";
        ss << "plt.text(0.99, 1.0 - 2 * lineHeight, 'Weighted median: ' + str(f'{weightedMedianValue:.4f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes, color=weightedMedianLineColor)\n\n";
        ss << "plt.text(0.99, 1.0 - 3 * lineHeight, 'Mean: ' + str(f'{meanValue:.4f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes, color=meanLineColor)\n\n";
        ss << "plt.text(0.99, 1.0 - 4 * lineHeight, '1st weighted quartile: ' + str(f'{firstWeightedQuartileValue:.4f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes, color=quartileLineColor)\n\n";
        ss << "plt.text(0.99, 1.0 - 5 * lineHeight, '3rd weighted quartile: ' + str(f'{thirdWeightedQuartileValue:.4f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes, color=quartileLineColor)\n\n";
        ss << "plt.text(0.99, 1.0 - 6 * lineHeight, 'Min: ' + str(f'{minValue:.4f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes)\n";
        ss << "plt.text(0.99, 1.0 - 7 * lineHeight, 'Max: ' + str(f'{maxValue:.4f}'), ha = 'right', fontsize = fontSize, transform = axes.transAxes)\n";

        ss << "axes.set_xlim(0.0, 1.0)\n";
        ss << "axes.set_ylim(0.0, maxY * 1.05)\n";
        ss << "axes.axvline(x = meanValue, color = meanLineColor, linewidth = 1.5)\n\n";
        ss << "axes.axvline(x = weightedMedianValue, color = weightedMedianLineColor, linewidth = 1.5)\n\n";
        ss << "axes.axvline(x = firstWeightedQuartileValue, color = quartileLineColor, linewidth = 1.5)\n\n";
        ss << "axes.axvline(x = thirdWeightedQuartileValue, color = quartileLineColor, linewidth = 1.5)\n\n";
        ss << "axes.axvline(x = minValue, color='black', linestyle = ':', linewidth = 1.5)\n\n";
        ss << "axes.axvline(x = maxValue, color='black', linestyle = ':', linewidth = 1.5)\n\n";

        ss << "if len(sys.argv) > 1:\n";
        ss << "\tif sys.argv[1] == '-save':\n";
        ss << "\t\tplt.savefig(sys.argv[2])\n";
        ss << "else:\n";
        ss << "\tplt.show()\n";

        ss << std::endl;

        return ss.str();
    }
};

class pooling
{
private:
    size_t mValueCount;
    double mValueSum;
    double mSquareValueSum;
    double mMinValue;
    double mMaxValue;
    double mVarianceBias = 0.0;
    uint32_t mMinCoord[2];
    uint32_t mMaxCoord[2];
    histogram mHistogram = histogram(100);
public:
    pooling() { clear(); }
    pooling(size_t buckets, double varianceBias = 0.0) : mVarianceBias(varianceBias) { mHistogram.resize(buckets); clear(); }
    
    histogram& getHistogram() { return mHistogram; }
    double getMinValue() const { return mMinValue; }
    double getMaxValue() const { return mMaxValue; }
    double getMean() const { return mValueSum / mValueCount; }
    double getVariance() const
    {
        bool isSampled = false;
        double variance = (mSquareValueSum - ((mValueSum * mValueSum) / mValueCount)) / (mValueCount - (isSampled ? 1 : 0));
        variance = std::max(0.0, variance);  //  negative not allowed, but can happen due to precision issues
        return variance;
    }

    double getStdDev(void) const { return sqrt(this->getVariance()); }

    double getCenterOfGravity(void) const 
    {
        double bucketStep = mHistogram.getBucketStep();
        double sumMoments = 0.0;
        double sumWeightedDataValue = 0.0;
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            double weight = (bucketId + 0.5) * bucketStep;
            double weightedValue = mHistogram.getBucketValue(bucketId) * weight;
            sumWeightedDataValue += weightedValue;
            sumMoments += weightedValue * weight;
        }
        double cog = sumMoments / sumWeightedDataValue;
        return cog;
    }

    double getWeightedPercentile(const double percent) const  // percent = 0.5 gives you the weighted median. This function is approximative, since we do not have all the values. We find the bucket where the weighted median is located, and interpolate in there.
    {
        double weight;
        double weightedValue;
        double bucketStep = mHistogram.getBucketStep();
        double sumWeightedDataValue = 0.0;
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            weight = (bucketId + 0.5) * bucketStep;
            weightedValue = mHistogram.getBucketValue(bucketId) * weight;
            sumWeightedDataValue += weightedValue;
        }

        double sum = 0;
        size_t weightedMedianIndex = 0;
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            weight = (bucketId + 0.5) * bucketStep;
            weightedValue = mHistogram.getBucketValue(bucketId) * weight;
            weightedMedianIndex = bucketId;
            if (sum + weightedValue > percent * sumWeightedDataValue)
                break;
            sum += weightedValue;
        }

        weight = (weightedMedianIndex + 0.5) * bucketStep;
        weightedValue = mHistogram.getBucketValue(weightedMedianIndex) * weight;
        double discrepancy = percent * sumWeightedDataValue - sum;
        double linearWeight = discrepancy / weightedValue; // in [0,1]
        double percentile = (weightedMedianIndex + linearWeight) * bucketStep;
        return percentile;
    }

    double getPercentile(const double percent, size_t numCounts) const  // percent = 0.5 gives you the median. This function is approximative, since we do not have all the values. We find the bucket where the median is located and returns its weight.
    {
        size_t sum = 0;
        size_t medianIndex = 0;
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            size_t count = mHistogram.getBucketValue(bucketId);
            medianIndex = bucketId;
            if (sum + count > size_t(percent * numCounts))
                break;
            sum += count;
        }
        double bucketStep = mHistogram.getBucketStep();
        double percentile = (medianIndex + 0.5) * bucketStep;
        return percentile;
    }

    void clear()
    {
        mValueCount = 0;
        mValueSum = 0.0;
        mSquareValueSum = 0.0;
        mMinValue = std::numeric_limits<double>::max();
        mMaxValue = std::numeric_limits<double>::min();
        mHistogram.clear();
    }

    void update(uint32_t xcoord, uint32_t ycoord, double value)
    {
        mValueCount++;
        mValueSum += value;
        mSquareValueSum += (value * value);
        mHistogram.inc(value);

        if (value < mMinValue)
        {
            mMinValue = value;
            mMinCoord[0] = xcoord;
            mMinCoord[1] = ycoord;
        }

        if (value > mMaxValue)
        {
            mMaxValue = value;
            mMaxCoord[0] = xcoord;
            mMaxCoord[1] = ycoord;
        }
    }

    std::string toString(bool verbose = false)
    {
        std::stringstream s;
        if (verbose)
        {
            s << std::fixed << std::setprecision(5) << "Mean = " << this->getMean() << ", Weighted median = " << this->getWeightedPercentile(0.5) << ", 1st weighted quartile = " << this->getWeightedPercentile(0.25) << ", 3rd weighted quartile = " << this->getWeightedPercentile(0.75) << ", Min value = " << mMinValue << " @ (" << mMinCoord[0] << "," << mMinCoord[1] << "), Max value =  " << mMaxValue << " @ (" << mMaxCoord[0] << ", " << mMaxCoord[1] << ")\n";
        }
        else
        {
            s << "Mean ; Weighted median ; 1st weighted quartile; 3rd weighted quartile ; Min value ; MinPosX; MinPosY; Max value; MaxPosX; MaxPosY\n";
            s << std::fixed << std::setprecision(5) << this->getMean() << ";" << this->getWeightedPercentile(0.5) << ";" << this->getWeightedPercentile(0.25) << ";" << this->getWeightedPercentile(0.75) << ";" << mMinValue << ";" << mMinCoord[0] << ";" << mMinCoord[1] << ";" << mMaxValue << ";" << mMaxCoord[0] << ";" << mMaxCoord[1] << "\n";
        }
        return s.str();
    }

    void save(const std::string fileName, float ppd, int imgWidth, int imgHeight, const bool verbose, const bool optionLog, const std::string referenceFileName, const std::string testFileName)
    {
        std::ofstream file;
        if (verbose)
        {
            std::cout << "Writing metric histogram to file <" << fileName << ".csv> and <" << fileName << ".py> with image resolution: " << imgWidth << "x" << imgHeight << "\n";
        }

        file.open(fileName + ".csv");
        file << "#  FLIP pooling statistics for <" << referenceFileName << "> vs. <" << testFileName << ">\n";

        file << this->toString(false);

        file << "#  histogram\n";
        file << "Min bucket" << ";" << "Max bucket" << "\n";
        file << mHistogram.getBucketIdMin() << ";" << mHistogram.getBucketIdMax() << "\n";
        file << "Bucket no; ";
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            file << bucketId << ";";
        }
        file << "\n";

        if (verbose)
        {
            std::cout << this->toString(true);
            std::cout << "# Histogram bucket size = " << mHistogram.getBucketSize() << ", range = [" << mHistogram.getBucketIdMin() << ", " << mHistogram.getBucketIdMax() << "]\n";
            std::cout << "Non-weighted bucket counts:\n";
        }

        size_t area = size_t(imgWidth) * size_t(imgHeight);

        file << "Count; ";
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            size_t histogramValue = mHistogram.getBucketValue(bucketId);
            if (verbose)
            {
                std::cout << bucketId << ": " << histogramValue << "\n";
            }
            file << float(histogramValue) << ";";
        }
        file << "\n";

        file << "Weight; ";
        double bucketStep = mHistogram.getBucketStep();
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            file << (0.5 + bucketId) * bucketStep << ";";
        }
        file << "\n";

        file << "Weighted bucket count per megapixel; ";
        double megaPixelWeight = 1024.0 * 1024.0 / double(area);
        for (size_t bucketId = 0; bucketId < mHistogram.size(); bucketId++)
        {
            file << ((0.5 + bucketId) * bucketStep * mHistogram.getBucketValue(bucketId) * megaPixelWeight) << ";";
        }
        file << "\n";

        //  python output
        std::ofstream pythonHistogramFile;
        pythonHistogramFile.open(fileName + ".py");
        pythonHistogramFile << mHistogram.toPython(area, ppd, getMean(), getStdDev(), getMaxValue(), getMinValue(), getWeightedPercentile(0.5), getWeightedPercentile(0.25), getWeightedPercentile(0.75),  optionLog);
        pythonHistogramFile.close();
    }
};