/*
 * sql.h
 *
 * Copyright 2014, 2015 John C. Bollinger
 *
 *
 * This file is part of the CIF API.
 *
 * The CIF API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CIF API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is an internal CIF API header file, intended for use only in building the CIF API library.  It is not needed
 * to build client applications, and it is intended to *not* be installed on target systems.
 */

/*
 * Macros for the text of DML and auxiliary SQL statements used by the library
 */

#ifndef INTERNAL_SQL_H
#define INTERNAL_SQL_H

#define ENABLE_FKS_SQL "pragma foreign_keys = 'on'; pragma foreign_keys"

#define CREATE_BLOCK_SQL "insert into data_block(container_id, name, name_orig) values (?, ?, ?)"

#define GET_BLOCK_SQL "select container_id as id, name_orig from data_block where name = ?"

#define GET_ALL_BLOCKS_SQL "select container_id as id, name, name_orig from data_block"

#define CREATE_FRAME_SQL "insert into save_frame(container_id, parent_id, name, name_orig) values (?, ?, ?, ?)"

#define GET_FRAME_SQL "select container_id as id, name_orig from save_frame where parent_id = ? and name = ?"

#define GET_ALL_FRAMES_SQL "select container_id as id, name, name_orig from save_frame where parent_id = ?"

#define VALIDATE_CONTAINER_SQL "select 1 from container where id = ?"

#define DESTROY_CONTAINER_SQL "delete from container where id = ?"

#define CREATE_LOOP_SQL "insert into unnumbered_loop (container_id, category) values (?, ?)"

#define DESTROY_LOOP_SQL "delete from loop where container_id = ? and loop_num = ?"

#define GET_LOOPNUM_SQL "select max(loop_num) from loop where container_id = ?"

#define SET_CATEGORY_SQL "update loop set category = ? where container_id = ? and loop_num = ?"

#define GET_CAT_LOOP_SQL "select loop_num from loop where container_id = ? and category = ?"

#define GET_ITEM_LOOP_SQL "select l.loop_num, l.category from loop l " \
        "join loop_item li on l.container_id = li.container_id and l.loop_num = li.loop_num " \
        "where li.container_id = ? and li.name = ?"

#define GET_ALL_LOOPS_SQL "select loop_num, category from loop where container_id = ?"

#define PRUNE_SQL "delete from loop where container_id = ? and loop_num not in " \
        "(select distinct loop_num from loop_item li join item_value using (container_id, name) where container_id = ?1)"

/*
 * This statement both updates existing values and sets omitted values in all packets of the loop containing the
 * specified name in the specified container:
 */
#define SET_ALL_VALUES_SQL "insert or replace into item_value " \
  "(kind, val_text, val, val_digits, su_digits, scale, container_id, name, row_num) " \
  "select ?, ?, ?, ?, ?, ?, ?, ?, loop_row.row_num " \
     "from (" \
       "select distinct iv.row_num as row_num " \
       "from loop_item li1 " \
         "join loop_item li2 on li1.container_id = li2.container_id and li1.loop_num = li2.loop_num " \
         "join item_value iv on li2.container_id = iv.container_id and li2.name = iv.name " \
       "where li1.container_id = ?7 and li1.name = ?8" \
     ") loop_row"

/* Loop "size" is the number of data names in a loop.  See also COUNT_LOOP_PACKETS_SQL. */
#define GET_LOOP_SIZE_SQL "select loop_num, count(*) as size " \
        "from loop_item li1 join loop_item li2 using (container_id, loop_num) " \
        "where li1.container_id = ? and li1.name = ? " \
        "group by loop_num"

/* not currently used: */
#define COUNT_LOOP_PACKETS_SQL "select count(*) as packet_count " \
  "from (" \
    "select distinct iv.row_num " \
      "from loop_item li " \
        "join item_value iv on li.container_id = iv.container_id and li.name = iv.name " \
      "where li.container_id = ? and li.loop_num = ?" \
  ")"

#define REMOVE_ITEM_SQL "delete from loop_item where container_id = ? and name = ?"

#define GET_LOOP_NAMES_SQL "select name_orig from loop_item where container_id = ? and loop_num = ?"

#define CHECK_ITEM_LOOP_SQL "select 1 from loop_item where container_id = ? and name = ? and loop_num = ?"

/*
 * This approach to assigning packet (row) numbers is in a sense more correct than one based on tracking a sequence
 * number in the 'loop' table as we now do, but it's too expensive for loops with large numbers of packets, especially
 * when used repeatedly.

#define MAX_PACKET_NUM_SQL "select max(iv.row_num) from loop_item li " \
    "join item_value iv using (container_id, name) where li.container_id = ? and li.loop_num = ?"

 */

#define GET_PACKET_NUM_SQL "select last_row_num from loop where container_id = ? and loop_num = ?"

#define UPDATE_PACKET_NUM_SQL "update loop set last_row_num = last_row_num + 1 where container_id = ? and loop_num = ?"

#define RESET_PACKET_NUM_SQL "update loop set last_row_num = 0 where container_id = ? and loop_num = ?"

#define ADD_LOOP_ITEM_SQL "insert into loop_item (container_id, name, name_orig, loop_num) values (?, ?, ?, ?)"

#define INSERT_VALUE_SQL "insert into item_value (container_id, name, row_num, " \
    "kind, val_text, val, val_digits, su_digits, scale) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define UPDATE_VALUE_SQL "insert or replace into item_value (container_id, name, row_num, " \
    "kind, val_text, val, val_digits, su_digits, scale) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define GET_VALUE_SQL "select kind, val, val_text, val_digits, su_digits, scale " \
        "from item_value where container_id = ? and name = ?"

/*
 * Note: there is no dedicated stmt in the cif struct corresponding to this SQL; a new statement is needed for each
 * loop iterated to allow multiple iterations to proceed simultaneously (as if doing that were a good idea ...)
 */
#define GET_LOOP_VALUES_SQL \
    "select iv.row_num, name, iv.kind, iv.val, iv.val_text, iv.val_digits, iv.su_digits, iv.scale " \
    "from loop_item li join item_value iv using (container_id, name) " \
    "where li.container_id=? and li.loop_num=? " \
    "order by iv.row_num"

#define REMOVE_PACKET_SQL "delete from item_value where container_id = ?1 and row_num = ?3 " \
        "and name in (select name from loop_item where container_id = ?1 and loop_num = ?2)"

#endif

