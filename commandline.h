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

#define NOMINMAX

#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <sstream>

template<typename T>
class unique_vector;

template<typename T>
class unique_vector_iterator
{
private:
    size_t mPosition;
    const unique_vector<T>* mpUniqueVector;

public:
    unique_vector_iterator(const unique_vector<T>* pVector, size_t position) : mPosition(position), mpUniqueVector(pVector) { }
    bool operator!=(const unique_vector_iterator& other) const { return this->mPosition != other.mPosition; }
    T operator*(void) const { return this->mpUniqueVector->at(this->mPosition); }
    const unique_vector_iterator& operator++(void) { this->mPosition++; return *this; }
    unique_vector_iterator operator+(size_t v) const { return unique_vector_iterator(this->mpUniqueVector, this->mPosition + v); }
};

template<typename T>
class unique_vector
{
public:
    typedef unique_vector_iterator<T> iterator;

private:
    std::vector<T> mVector;

public:
    void push_back(T value) { if (!this->contains(value)) this->mVector.push_back(value); }
    T at(size_t index) const { return this->mVector.at(index); }
    T operator[](size_t index) const { return this->mVector.at(index); }
    T& operator[](size_t index) { return this->mVector.at(index); }
    size_t size(void) const { return this->mVector.size(); }
    bool contains(T value) const { return std::find(this->mVector.begin(), this->mVector.end(), value) != mVector.end(); }

    iterator begin(size_t begin = 0) const { return iterator(this, begin); }
    iterator end(size_t end = std::string::npos) const { return iterator(this, (end != std::string::npos && end <= mVector.size() ? end : mVector.size())); }

    // conversion to const std::vector
    operator const std::vector<T>() const { return mVector; }
    operator std::vector<T>() { return mVector; }
};

typedef std::set<std::pair<std::string, bool>> commandline_options;

class commandline
{
private:
    std::string mCommand;
    std::map<std::string, unique_vector<std::string>> mOptions;
    std::vector<std::string> mArguments;

public:
    commandline()
    {
        clear();
    }
    commandline(int argc, char* argv[], const commandline_options& allowedOptions = {})
    {
        clear();
        parse(argc, argv, allowedOptions);
    }
    commandline(std::string commandLine, const commandline_options& allowedOptions = {})
    {
        clear();
        parse(commandLine, allowedOptions);
    }

    void clear(void)
    {
        mCommand = "";
        mOptions.clear();
        mArguments.clear();
    }

    bool parse(int argc, char* argv[], const commandline_options& allowedOptions = {})
    {
        mCommand = argv[0];

        bool atOption = false;
        bool optionHasArgument = false;
        std::string tOption = "";
        for (int i = 1; i < argc; ++i)
        {
            std::string arg(argv[i]);

            if (arg[0] == '-')
            {
                if (atOption)
                {
                    this->mOptions[tOption].push_back("");
                }

                if (allowedOptions.find({ arg.substr(1), false }) != allowedOptions.end())
                {
                    atOption = true;
                    tOption = arg.substr(1);
                }
                else if (allowedOptions.find({ arg.substr(1), true }) != allowedOptions.end())
                {
                    atOption = true;
                    optionHasArgument = true;
                    tOption = arg.substr(1);
                }
                else
                {
                    atOption = false;
                    this->mArguments.push_back(arg);
                }
            }
            else
            {
                if (atOption && optionHasArgument)
                {
                    size_t pos = 0;
                    std::string token;
                    std::string delimiter = ",";
                    while ((pos = arg.find(delimiter)) != std::string::npos) {
                        token = arg.substr(0, pos);
                        this->mOptions[tOption].push_back(token);
                        arg.erase(0, pos + delimiter.length());
                    }
                    this->mOptions[tOption].push_back(arg);

                    atOption = false;
                    optionHasArgument = false;
                }
                else
                {
                    this->mArguments.push_back(arg);
                }
            }
        }
        if (atOption)
        {
            this->mOptions[tOption].push_back("");
        }
        return true;
    }

    bool parse(std::string commandLine, const commandline_options& allowedOptions = {})
    {
        std::vector<std::string> vCommandLine;
        std::string command;
        std::stringstream ss(commandLine);

        while (ss >> command)
            vCommandLine.push_back(command);

        std::vector<char*> vpCommandLine;
        for (size_t i = 0; i < vCommandLine.size(); i++)
            vpCommandLine.push_back(const_cast<char *>(vCommandLine[i].c_str()));

        return parse(int(vpCommandLine.size()), &vpCommandLine[0], allowedOptions);
    }

    inline const std::string& getCommand(void) const
    {
        return mCommand;
    }

    inline size_t getNumArguments(void) const
    {
        return this->mArguments.size();
    }

    inline const std::string getArgument(size_t index) const
    {
        assert(index < this->mArguments.size());
        return this->mArguments[index];
    }

    const std::vector<std::string>& getArguments(void) const
    {
        return this->mArguments;
    }

    std::string getOptionValue(const std::string& option) const
    {
        static const std::string emptyValue;

        if (this->mOptions.find(option) == this->mOptions.end())
            return emptyValue;

        return this->mOptions.at(option)[0];
    }

    const std::vector<std::string>& getOptionValues(const std::string& option) const
    {
        static const std::vector<std::string> emptyValues;

        if (this->mOptions.find(option) == this->mOptions.end())
            return emptyValues;

        return (std::vector<std::string>&)(this->mOptions.at(option));
    }

    const std::string& getOption(const std::string& option, size_t index) const
    {
        static const std::string emptyString;

        auto options = this->getOptionValues(option);

        if (index < options.size())
            return options[index];

        return emptyString;
    }

    size_t getNumOptionValues(const std::string& option) const
    {
        return this->getOptionValues(option).size();
    }

    bool optionSet(const std::string& option) const
    {
        return this->mOptions.find(option) != this->mOptions.end();
    }
};
