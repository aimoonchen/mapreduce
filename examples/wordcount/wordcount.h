// Copyright (c) 2009-2013 Craig Henderson
// https://github.com/cdmh/mapreduce

#include <numeric>      // accumulate

namespace wordcount {

struct map_task : public mapreduce::map_task<
                             std::string,                               // MapKey (filename)
                             std::pair<char const *, std::uintmax_t> >  // MapValue (memory mapped file contents)
{
    template<typename Runtime>
    void operator()(Runtime &runtime, key_type const &/*key*/, value_type &value) const
    {
        bool in_word = false;
        char const *ptr = value.first;
        char const *end = ptr + value.second;
        char const *word = ptr;
        for (; ptr != end; ++ptr)
        {
            char const ch = std::toupper(*ptr, std::locale::classic());
            if (in_word)
            {
                if ((ch < 'A' || ch > 'Z') && ch != '\'')
                {
                    runtime.emit_intermediate(std::make_pair(word,ptr-word), 1);
                    in_word = false;
                }
            }
            else if (ch >= 'A'  &&  ch <= 'Z')
            {
                word = ptr;
                in_word = true;
            }
        }

        if (in_word)
        {
            assert(ptr > word);
            runtime.emit_intermediate(std::make_pair(word,ptr-word), 1);
        }
    }
};

struct reduce_task : public mapreduce::reduce_task<
                                std::pair<char const *, std::uintmax_t>,
                                unsigned>
{
    template<typename Runtime, typename It>
    void operator()(Runtime &runtime, key_type const &key, It it, It const ite) const
    {
        runtime.emit(key, std::accumulate(it, ite, 0));
    }
};

class combiner
{
  public:
    template<typename IntermediateStore>
    static void run(IntermediateStore &intermediate_store)
    {
        combiner instance;
        intermediate_store.combine(instance);
    }

    void start(reduce_task::key_type const &)
    {
        total_ = 0;
    }

    template<typename IntermediateStore>
    void finish(reduce_task::key_type const &key, IntermediateStore &intermediate_store)
    {
        if (total_ > 0)
            intermediate_store.insert(key, total_);
    }

    void operator()(reduce_task::value_type const &value)
    {
        total_ += value;
    }
        
  private:
    combiner() { }

  private:
    unsigned total_;
};

}   // namespace wordcount

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
