/*
  Copyright (C) 2022 by Gotlim
 
  This file is part of soci-test.
 
  soci-test is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.
 
  soci-test is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with
  soci-test. If not, see <https://www.gnu.org/licenses/>. */

/* Written by Jeremy Fonseca */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <getopt.h>
#include <soci/soci.h>
#include <soci/statement.h>
#include <soci/sqlite3/soci-sqlite3.h>

#define PROGNAME "soci-test"

static const struct option long_options[] = {
  {"database", required_argument, NULL, 'd'},
  {"insert", required_argument, NULL, 'i'},
  {"select", no_argument, NULL, 's'},
  {"help", no_argument, NULL, 0},
  {NULL, 0, NULL, 0}
};

struct exam_t
{
  uint64_t id;
  std::string name;
  double price;
  unsigned short is_edited;
  unsigned short is_deleted;
};

struct program_mode
{
  bool insert_mode{ false };
  bool select_mode{ false };
};

void usage(int32_t status)
{
  if (status == EXIT_SUCCESS)
  {
    std::fprintf(stderr, "Try `%s --help' for more information.\n", PROGNAME);
  }
  else
  {
    std::printf("Usage: %s -d NAME [OPTION] RECORDS\n"\
    "Test client for SQLite3 using SOCI.\n\n"
    "Mandatory arguments to long options are mandatory for short options too.\n"\
    "  -d, --database         selects the database to work with\n"
    "  -i, --insert=n         insert n records into database NAME\n"\
    "  -s, --select           select all records in database NAME\n\n"\
    "Arguments:\n"\
    "  NAME                   The name of the database to work with\n"
    "  RECORDS                Positive number of records to work with\n\n"
    "Written by Jeremy Fonseca for Gotlim.\n",
    PROGNAME);
  }
  exit(status);
}

int32_t main(int32_t argc, char** argv)
{
  int8_t c{};
  program_mode x;

  int records{};
  std::string db_name{};

  while (c = getopt_long(argc, argv, "i:d:hs", long_options, NULL), c ^ -1)
  {
    if (c == '?')
    {
      usage(EXIT_FAILURE);
    }
    
    switch (c)
    {

      case 'd':
      {
        db_name = optarg;
        break;
      }

      case 'i':
      {
        x.insert_mode = true;
        records = std::atoi(optarg);
        break;
      }
      
      case 's':
      {
        x.select_mode = true;
        break;
      }
      
      case 'h':
      {
       usage(EXIT_FAILURE);
      }

      // default:
      // {
      //   usage(EXIT_FAILURE);
      // }
    }
  }

  if (x.insert_mode && records < 1)
  {
    usage(EXIT_FAILURE);
  }

  /* There were no options. */

  if (argc == 1)
  {
    usage(EXIT_SUCCESS);
  }

  soci::session sql(soci::sqlite3, db_name + ".db");

  if (!sql.is_connected())
  {
    std::fprintf(stderr, "%s: could not connect to database %s.\n", PROGNAME, db_name.c_str());
    std::cerr << "Could not connect to database." << '\n';
    exit(EXIT_FAILURE);
  }

  sql << "create table if not exists exam(id integer, name text, price real, is_edited integer, is_deleted integer);";

  if (x.insert_mode)
  {

    /* Get the number of exams for incrementation */

    int32_t examn_count{};

    sql << "select count(*) from exam", soci::into(examn_count);

    /* Insert data into table. */

    for (std::size_t i = 1; i <= records; ++i)
    {
      exam_t exam;

      exam.id = i + examn_count;

      std::cout << "Exam name: ";
      std::getline(std::cin, exam.name);

      std::cout << "Exam price: ";
      std::cin >> exam.price;

      std::cout << "Is edited? (1/0) ";
      std::cin >> exam.is_edited;

      std::cout << "Is deleted? (1/0) ";
      std::cin >> exam.is_deleted;

      std::cin.ignore(std::numeric_limits<int32_t>::max(), '\n');
      std::cin.clear();

      soci::statement st = (
        sql.prepare << "insert into exam(id, name, price, is_edited, is_deleted) values(:i, :n, :p, :is_e, :is_d)",
        soci::use(exam.id, "i"),
        soci::use(exam.name, "n"),
        soci::use(exam.price, "p"),
        soci::use(exam.is_edited, "is_e"),
        soci::use(exam.is_deleted, "is_d"));

      st.execute(true);
    }
  }

  if (x.select_mode)
  {
    /* List to automatially display the header of
      the current table. */

    std::list<std::string> col_header_list{};
    soci::row r;

    sql <<  "select * from exam", soci::into(r);

    int32_t max_col_length = std::numeric_limits<int32_t>::min();

    /* Iterate a radom row to get the column with
      the maxinum length to later use as a base for
      std::cout width. */

    for (std::size_t i = 0; i != r.size(); ++i)
    {
      const soci::column_properties& properties = r.get_properties(i);
      const std::string current_col_name = properties.get_name();
      int32_t current_col_name_length = current_col_name.length();
      col_header_list.push_back(current_col_name);
      max_col_length = (current_col_name_length > max_col_length)
        ? current_col_name_length
        : max_col_length;
    }

    int width = max_col_length + 2;

    /* Now, keep a stream of all the columns. */

    std::stringstream table_header_stream;

    for (const std::string& col_name : col_header_list)
      table_header_stream << std::setw(width) << col_name;

    table_header_stream << '\n';
    std::cout << table_header_stream.str();

    /* Display each row of the table. */

    soci::rowset<soci::row> rset = (sql.prepare << "select * from exam");

    for (soci::rowset<soci::row>::const_iterator r = rset.begin(); r != rset.end(); ++r)
    {
      for (std::size_t i = 0; i != r->size(); ++i)
      {
        const soci::column_properties& properties = r->get_properties(i);
        std::stringstream ss;
        
        switch (properties.get_data_type())
        {
          case soci::dt_double:
            ss << std::setw(width) << r->get<double>(i);
            break;
          case soci::dt_string:
            ss << std::setw(width) << r->get<std::string>(i);
            break;
          case soci::dt_integer:
            ss << std::setw(width) << r->get<int>(i);
            break;
        }

        std::cout << ss.str();
      }
      std::putchar(10);
    }
  }

  return EXIT_SUCCESS;
}