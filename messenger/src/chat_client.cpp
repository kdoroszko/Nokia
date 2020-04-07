//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <deque>
#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
  enum { send_msg_cmd_length = 10 };

  chat_client(boost::asio::io_context& io_context,
      const tcp::resolver::results_type& endpoints)
    : io_context_(io_context),
      socket_(io_context)
  {
    do_connect(endpoints);
  }

  void write(const chat_message& msg)
  {
    boost::asio::post(io_context_,
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

private:
  void do_connect(const tcp::resolver::results_type& endpoints)
  {
    boost::asio::async_connect(socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    std::string send_file_(write_msgs_.front().body(), send_msg_cmd_length);

    if((send_file_.compare(std::string{"/sendfile "})) == 0)
    {
      std::size_t path_length = std::strlen(write_msgs_.front().body() + send_msg_cmd_length);
      std::string path_to_file(write_msgs_.front().body() + send_msg_cmd_length, path_length - 1);
      //TODO: oblsuzyc brak podania sciezki
      // std::cout << std::quoted(path_to_file_);
      std::fstream file_to_send(path_to_file);
      std::experimental::filesystem::path path = path_to_file; //TODO: obsluzyc wyjatki, gdy nie ma takiej sciezki
      std::size_t file_size = std::experimental::filesystem::file_size(path);

      char temp_header[chat_message::header_length + 1] = "";
      std::sprintf(temp_header, "%4d", static_cast<int>(file_size));
      std::string new_header(temp_header);

      file_to_send.open(path_to_file, std::ios::in);
      std::string file_message;
      while(!file_to_send.eof())
        file_to_send >> file_message;
      file_to_send.close();
      std::string header_with_file = temp_header + file_message;
      std::fstream send(header_with_file);

      do_write_file(send, chat_message::header_length + file_size);
      //TODO: dokonczyc metode wysylajaca pliki, dopisac metode zapisujaca plik na dysku
    }
    else
    {
      do_write_text();
    }
  }

  void do_write_file(std::fstream& new_file, std::size_t new_length)
  {
    boost::asio::async_write(socket_,
          boost::asio::buffer(new_file.rdbuf(), new_length),
          [this](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (!ec)
            {
              write_msgs_.pop_front();
              if (!write_msgs_.empty())
              {
                do_write();
              }
            }
            else
            {
              socket_.close();
            }
          });
  }

  void do_write_text()
  {
    boost::asio::async_write(socket_,
          boost::asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          [this](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (!ec)
            {
              write_msgs_.pop_front();
              if (!write_msgs_.empty())
              {
                do_write();
              }
            }
            else
            {
              socket_.close();
            }
          });
  }

private:
  boost::asio::io_context& io_context_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);

    std::thread t([&io_context](){ io_context.run(); });

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      chat_message msg(line, std::strlen(line));
      c.write(msg);
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
