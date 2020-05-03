#include "tee_output.h"

#include <fstream>
#include <iostream>
#include <streambuf>

namespace logging
{
  namespace detail
  {
    class FileOutput
    {
    public:
      FileOutput(const std::string_view& filename) : out_stream(filename.data())
      {
        if (!out_stream)
          throw std::runtime_error("Unable to open file: "
                                   + std::string(filename));
      }

      void put(std::basic_streambuf<char>::int_type ch)
      {
        out_stream.rdbuf()->sputc(static_cast<char>(ch));
      }

    private:
      std::ofstream out_stream;
    };

    class CapturingRdbuf : public std::basic_streambuf<char>
    {
    public:
      CapturingRdbuf(FileOutput& output_, std::ostream& stream_) :
        output(output_), stream(stream_)
      {
        orig_rdbuf = stream.rdbuf(this);
      }
      ~CapturingRdbuf() { stream.rdbuf(orig_rdbuf); }

      std::basic_streambuf<char>::int_type overflow(int_type ch) override
      {
        orig_rdbuf->sputc(static_cast<char>(ch));
        output.put(ch);
        return 0;
      }

    private:
      FileOutput& output;
      std::ostream& stream;
      std::basic_streambuf<char>* orig_rdbuf;
    };

  }  // namespace detail

  TeeOutput::TeeOutput(std::string_view filename) :
    file_output(std::make_unique<detail::FileOutput>(filename)), rdbufs()
  {
    rdbufs.push_back(
      std::make_unique<detail::CapturingRdbuf>(*file_output, std::cout));
    rdbufs.push_back(
      std::make_unique<detail::CapturingRdbuf>(*file_output, std::cerr));
    rdbufs.push_back(
      std::make_unique<detail::CapturingRdbuf>(*file_output, std::clog));
  }

  TeeOutput::~TeeOutput() {}

}  // namespace logging
