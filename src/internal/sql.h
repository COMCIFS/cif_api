/*
 * sql.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

/*
 * Macros for the text of DML and auxiliary SQL statements used by the library
 */

#ifndef INTERNAL_SQL_H
#define INTERNAL_SQL_H

#define ENABLE_FKS_SQL "pragma foreign_keys = 'on'; pragma foreign_keys"

#define CREATE_BLOCK_SQL "insert into data_block(container_id, name, name_orig) values (?, ?, ?)"

#define GET_BLOCK_SQL "select container_id as id, name_orig from data_block where name = ?"

#define CREATE_FRAME_SQL "insert into save_frame(container_id, data_block_id, name, name_orig) values (?, ?, ?, ?)"

#define GET_FRAME_SQL "select container_id as id, name_orig from data_block where data_block_id = ? and name = ?"

#define DESTROY_CONTAINER_SQL "delete from container where id = ?"

#define CREATE_LOOP_SQL "insert into loop (container_id, category, loop_num) values (?, ?, NULL)"

#define DESTROY_LOOP_SQL "delete from loop where container_id = ? and loop_num = ?"

#define GET_LOOPNUM_SQL "select max(loop_num) from loop where container_id = ?"

#define ADD_LOOPITEM_SQL "insert into loop_item(container_id, loop_num, name, name_orig) values (?, ?, ?, ?)"

#define GET_CAT_LOOP_SQL "select loop_num from loop where container_id = ? and category = ?"

#define GET_ITEM_LOOP_SQL "select l.loop_num, l.category from loop l " \
        "join loop_item li on l.container_id = li.container_id and l.loop_num = li.loop_num " \
        "where li.container_id = ? and li.name = ?"

#define GET_VALUE_SQL "select kind, val, val_text, val_digits, su_digits, scale " \
        "from item_value where container_id = ? and name = ?"

/*
#define SET_ALL_VALUES_SQL "update item_value " \
        "set kind = ?, val_text = ?, val = ?, val_digits = ?, su_digits = ? scale = ?" \
        "where container_id = ? and name = ?"
*/
/*
 * This version of the set-all-values statement both updates existing values and sets omitted values in all packets
 * of the loop containing the specified name in the specified container:
 */
#define SET_ALL_VALUES_SQL "insert or replace into item_value " \
  "(kind, val_text, val, val_digits, su_digits, scale, container_id, name, row_num) " \
  "select distinct ?, ?, ?, ?, ?, ?, ?7, ?8, iv.row_num " \
     "from loop_item li1 " \
       "join loop_item li2 on li1.container_id = li2.container_id and li1.loop_num = li2.loop_num " \
       "join item_value iv on li2.container_id = iv.container_id and li2.name = iv.name " \
     "where li1.container_id = ?7 and li1.name = ?8"

#define GET_LOOP_SIZE_SQL "select loop_num, count(*) as size " \
        "from loop_item li1 join loop_item li2 using (container_id, loop_num) " \
        "where container_id = ? and li1.name = ? " \
        "group by loop_num"

#define REMOVE_ITEM_SQL "delete from loop_item where container_id = ? and name = ?"

#define GET_LOOP_NAMES_SQL "select name from loop_item where container_id = ? and loop_num = ?"

#define CHECK_ITEM_LOOP_SQL "select 1 from loop_item where container_id = ? and name = ? and loop_num = ?"

#define MAX_PACKET_NUM_SQL "select max(iv.row_num) from loop_item li " \
    "join item_value iv using (container_id, name) where container_id = ? and li.loop_num = ?"

#define ADD_LOOP_ITEM_SQL "insert into loop_item (container_id, name, name_orig, loop_num) values (?, ?, ?, ?)"

#define INSERT_VALUE_SQL "insert into item_value (container_id, name, row_num, " \
    "kind, val_text, val, val_digits, su_digits, scale) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"

/*
 * Note: no dedicated stmt in the cif struct for these statements:
 */
#define GET_LOOP_VALUES_SQL \
    "select iv.row_num, name, iv.kind, iv.val, iv.val_text, iv.val_digits, iv.su_digits, iv.scale " \
    "from loop_item li join item_value iv using (container_id, name) " \
    "where container_id=? and loop_num=? " \
    "order by iv.row_num"

#define UPDATE_PACKET_ITEM_SQL "update item_value " \
        "set kind = ?, val_text = ?, val = ?, val_digits = ?, su_digits = ?, scale = ? " \
        "where container_id = ? and name = ? and row_num = ?"

#define INSERT_PACKET_ITEM_SQL "insert into item_value(kind, val_text, val, val_digits, su_digits, scale, " \
        "container_id, name, row_num) " \
        "values (?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define REMOVE_PACKET_SQL "delete from item_value where container_id = ?1 and row_num = ?3 " \
        "and name in (select name from loop_item where container_id = ?1 and loop_num = ?2)"

#endif

