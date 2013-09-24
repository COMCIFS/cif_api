--
-- Database schema for arbitrary CIF
--
-- Copyright (C) 2013 John C. Bollinger.  All rights reserved.
--
-- This schema relies on and represents only the general, underlying data model
-- of CIF.  As a design criterion, it is a closed schema, in the sense that it
-- defines all tables necessary to model any CIF.  In particular, it stores all
-- data values in the same table, rather than in separate per-loop tables,
-- because there is no way to predict which per-loop tables would be needed for
-- arbitrary CIF.
--
-- The database defined by this schema represents one logical CIF, organized at
-- the topmost level as a collection of data blocks.  Separate CIFs require
-- separate databases (but not necessarily separate DBMSs).
--
-- This schema relies to some extent on the characteristics of SQLite, especially
-- dynamic typing.  SQLite does not enforce limits on values' sizes based on their
-- declared data types (e.g. varchar(80)).
--
-- The text of this schema is processed at build time into a collection of C
-- string literals.  So that this is feasible without a full-blown SQL parser,
-- the following constraints must be followed:
-- 1. begin ... end blocks may not be nested.
-- 2. "end" keywords terminating blocks must be followed by semicolons on the
--    same line.
-- 3. Only whitespace may appear after "end;" on any line.
-- 4. The character sequence "begin", in any case combination, must not appear
--    except as the opening "begin" keyword of a block, or as the short form
--    of "begin transaction".  A semicolon must follow on the same line in the
--    latter case, and must NOT follow anywhere on the same line in the former.
-- 5. Only space characters and line terminators may be used as whitespace.
--
-- To make the process a little easier on the build system, the following is also
-- required:
-- 6. Comment lines must not end with semicolons.
--

--
-- Represents a data block or save frame to which data items may belong.  A
-- corresponding row is expected to exist (be created) in either the data_block
-- table or the save_frame table.
-- 
create table container (
  id integer primary key autoincrement,
  next_loop_num integer not null default 0
);

--
-- Represents a container that is a data block, rather than a save frame.  All
-- data blocks in the database must have distinct, nonempty names
--
create table data_block (
  container_id integer not null,
  name varchar(80) not null,
  name_orig varchar(80) not null,
  
  primary key (container_id),
  foreign key (container_id)
    references container(id)
    on delete cascade,
  unique (name)
);

--
-- Represents a container that is a save frame, rather than a data block.  Each
-- is associated with a data block that contains it.  All save frames
-- associated with the same data block must have distinct, nonempty names.
--
-- In the event that nested save frames are added to CIF, the container_id
-- foreign key can be pointed at the 'container' table instead of 'data_block'.
--
create table save_frame (
  container_id integer not null,
  data_block_id integer not null,
  name varchar(80) not null,
  name_orig varchar(80) not null,
  
  primary key (container_id),
  foreign key (container_id)
    references container(id)
    on delete cascade,
  foreign key (data_block_id)
    references data_block(container_id)
    on delete cascade,
  unique (data_block_id, name)
);

create index ix1_save_frame
  on save_frame (data_block_id);

--
-- Represents loops (including implicit ones) present in any container.
-- Loops associate data items into groups and their associated values into
-- packets.  The CIF (or other) category to which the data in the loop are
-- expected to belong can also be recorded here, if it is known.
--
create table loop (
  container_id integer not null,
  loop_num integer not null,
  category varchar(80),
 
  primary key (container_id, loop_num),
  foreign key (container_id)
    references container(id)
    on delete cascade
);

create index ix1_loop
  on loop(container_id, category);

--
-- Restrict each container to one scalar loop on insert
--
create trigger tr1_loop
  before insert on loop
  when NEW.category = ''
  begin
    select raise(ABORT, 'duplicate scalar loop')
      from loop
      where container_id = NEW.container_id and category = '';
  end;

--
-- Restrict each container to one scalar loop on update
--
create trigger tr2_loop
  before update of category on loop
  when (NEW.category = '') and (OLD.category <> '')
  begin
    select raise(ABORT, 'duplicate scalar loop')
      from loop
      where container_id = NEW.container_id and category = '';
  end;

--
-- This view merely provides a place to hang an "instead of" trigger
-- facilitating automatic generation of loop numbers within each container.
-- If loops are inserted only via this view, then loop numbers are
-- guaranteed to increase monotonically within each container.
--
create view unnumbered_loop as
  select container_id, category
  from loop;

create trigger tr1_unnumbered_loop
  instead of insert on unnumbered_loop
  begin
--  Substitute an insert statement on loop, with a non-null loop number
    insert into loop(container_id, loop_num, category)
      values (NEW.container_id,
--      The following evaluates to NULL if there is no container with the given
--      id; that's no problem, because in that case the insertion was already
--      going to fail
        (select next_loop_num from container where id = NEW.container_id),
        NEW.category);
    update container
      set next_loop_num = next_loop_num + 1
      where id = NEW.container_id;
  end;

--
-- Represents an item tabulated in a given loop
--
create table loop_item (
  container_id integer not null,
  name varchar(80) not null,
  name_orig varchar(80) not null,
  loop_num integer not null,
  
  primary key (container_id, name),
  foreign key (container_id, loop_num)
    references loop(container_id, loop_num)
    on delete cascade
);

create index ix1_loop_item
  on loop_item (container_id, loop_num);

--
-- Represents a single data value; particularly, the one for item 'name' in
-- packet number 'row' of the loop identified by 'loop_id'.
--
-- The 'kind' column encodes the data type of the value:
-- 0 = CHARACTER
-- 1 = NUMBER
-- 2 = LIST
-- 3 = TABLE
-- 4 = N/A
-- 5 = UNKNOWN
-- Only when the kind is one of the last two may 'val' be null
--
-- val_text records a text representation of CHARACTER and NUMBER values,
-- whereas val records a parsed value specific to the value kind: a copy of
-- the the value text for kind 0, a parsed double value for kind 1, or a
-- serialized list or table for kinds 2 and 3, respectively.  Both val and
-- val_text are NULL for kinds 4 and 5.
-- 
-- For kind 1, val_digits and su_digits record decimal digit-string
-- representations of the value and its standard uncertainty, with the
-- rightmost digit of each being interpreted as appearing in the
-- 10^(-scale) position.  These support arbitrary-precision fixed point
-- numbers of any scale expressible in CIF, but not necessarily in the
-- floating-point or integer format in which the DB stores numeric values.
-- su_digits is NULL for exact numbers.  The sign associated with the
-- (digit string, scale) representation of the value is taken from the
-- beginning of value_text; there is no separate field for it.
-- These fields are null for kinds other than 1.
--
-- IMPORTANT: If the precision or scale of a numeric CIF value exceeds that
-- representable in the floating-point format used by the DB engine (8-byte
-- IEEE floating point for SQLite 3 on x86 and x86_64) then val will at best
-- fail to express that value to its correct precision.  In such a case it
-- might also be denormalized, or it might express an infinite or a
-- not-a-number value.
--
-- Note also that this schema does not enforce the presence of an explicit value
-- for each item in each loop packet.  Where a loop contains an item (as defined
-- by table loop_item) but no value is given for it in this table for any
-- particular row_num, it should be interpreted as if a value existed in that
-- row with kind = 5, except as described next.
--
-- Row / packet numbers are not guaranteed to be consecutive for any given
-- loop.  Where there is a gap between row numbers for a given loop, it
-- means only that there are no packets having the corresponding number.
-- In particular, it does NOT imply the existence of any packets with
-- all-unknown values (though such packets can be represented explicitly).
--
-- Rationale:
-- The structure of this table addresses these design considerations:
-- 1. It must record in full fidelity all values representable via the
--    data types used by the CIF API, including numeric values that cannot
--    be expressed exactly as doubles.
-- 2. It must readily support selection predicates pertaining to the character
--    or numeric values represented by rows.
-- 3. Deletions from this table must suffice to effect loop packet deletions --
--    row updates must not be required.
-- 4. Some denormalization is preferrable to requiring common queries to
--    perform extra joins, especially outer joins.
-- 5. Selection predicates pertaining to the contents of list or table values
--    do NOT need to be readily supported by this table.
--
create table item_value (
  container_id integer not null,
  name varchar(80) not null,
  row_num integer not null,
  kind integer(1),
  -- specific to kinds 0 - 3:
  val numeric,
  -- specific to kinds 0 and 1:
  val_text varchar(80),
  -- specific to kind 1:
  val_digits varchar(15),
  su_digits varchar(15),
  scale integer(4),
  
  primary key (container_id, name, row_num),
  foreign key (container_id, name)
    references loop_item(container_id, name)
    on update cascade
    on delete cascade,
  check (row_num > 0),
  check (case when (val is null) then (kind in (4, 5)) else (kind not in (4, 5)) end),
  check (case when (val_text is null) then (kind not in (0, 1)) else (kind in (0, 1)) end),
  check (case when (kind = 1) then (scale is not null)
        and (length(val_digits) > 0) and (val_digits not glob '*[!0-9]*')
        and ((su_digits is null) or ((length(su_digits) > 0) and (su_digits not glob '*[!0-9]*')))
    else coalesce(val_digits, su_digits, scale) is null end)
);

--
-- Restrict scalar loops to one row on value insert
--
create trigger tr1_item_value
  before insert on item_value
  when NEW.row_num > 1
  begin
    select ifnull(nullif('', l.category), raise(ABORT, 'Attempted to create multiple values for a scalar'))
      from loop l
        join loop_item li using (container_id, loop_num)
        where container_id = NEW.container_id
          and li.name = NEW.name;
  end;

--
-- Restrict scalar loops to one row on value update
--
create trigger tr2_item_value
  before update of row_num on item_value
  when (OLD.row_num = 1) and (NEW.row_num <> 1)
  begin
    select ifnull(nullif('', l.category), raise(ABORT, 'Attempted to create multiple values for a scalar'))
      from loop l
        join loop_item li using (container_id, loop_num)
        where container_id = NEW.container_id
          and li.name = NEW.name;
  end;

--
-- Parse-time diagnostic messages
-- 
create table error (
  id integer primary key autoincrement,
  line integer not null,
  col integer not null,
  message varchar(80) not null
);

