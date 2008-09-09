create table data (
    id int unsigned auto_increment,
    title varchar(255) not null,
    description text,
    att_float float not null default 0.0,
    att_uint integer unsigned not null default 0,
    att_group integer unsigned not null,
    att_timestamp timestamp not null,
    att_bool bool not null,
    primary key(id)
);



insert into data values (1, 'ahoj cece',  'popisek pes kocka morce', 0.5, 10, 5, unix_timestamp(NOW()), true);
insert into data values (2, 'ahoj bla',   'popisek pes kocka morce', 1.4, 45, 5, unix_timestamp(NOW()), false);
insert into data values (3, 'ahoj xxx',   'pes kocka morce',         2.3, 67, 5, unix_timestamp(NOW()), true);
insert into data values (4, 'ahoj iii',   'popisek kocka morce',     3.5, 34, 5, unix_timestamp(NOW()), false);
insert into data values (5, 'ahoj prd',   'popisek pes kocka',       4.1, 23, 5, unix_timestamp(NOW()), true);
insert into data values (6, 'ahoj ble',   'popisek pes morce',       5.2, 62, 5, unix_timestamp(NOW()), true);
insert into data values (7, 'ahoj www',   'popisek kocka morce',     6.3, 56, 5, unix_timestamp(NOW()), false);
insert into data values (8, 'ahoj zzz',   'popisek pes kocka ',      7.4, 67, 6, unix_timestamp(NOW()), true);
insert into data values (9, 'ahoj cvasr', 'pes kocka morce',         8.5, 23, 6, unix_timestamp(NOW()), true);
insert into data values (10, 'ahoj fawe', 'popisek kocka morce',     9.6, 98, 6, unix_timestamp(NOW()), false);
insert into data values (11, 'ahoj bqrh', 'pes kocka morce',         0.7, 45, 6, unix_timestamp(NOW()), true);
insert into data values (12, 'ahoj mcaos', 'popisek pes',            1.8, 23, 7, unix_timestamp(NOW()), true);
insert into data values (13, 'ahoj bapjr', 'popisek morce',          2.9, 12, 7, unix_timestamp(NOW()), false);
insert into data values (14, 'ahoj v;la', 'popisek kocka',           3.0, 56, 7, unix_timestamp(NOW()), false);
insert into data values (15, 'ahoj dddd', 'morce',                   4.5, 10, 7, unix_timestamp(NOW()), true);

create table multi_attribute_data (
    data_id int unsigned not null default 0,
    int_val int unsigned not null default 0
);

insert into multi_attribute_data values (1, 1);
insert into multi_attribute_data values (1, 3);
insert into multi_attribute_data values (1, 4);
insert into multi_attribute_data values (2, 7);
insert into multi_attribute_data values (2, 9);
insert into multi_attribute_data values (3, 0);
insert into multi_attribute_data values (3, 9);
insert into multi_attribute_data values (3, 8);
insert into multi_attribute_data values (4, 7);
insert into multi_attribute_data values (5, 6);
insert into multi_attribute_data values (7, 5);
insert into multi_attribute_data values (8, 4);
insert into multi_attribute_data values (8, 3);
insert into multi_attribute_data values (8, 2);
insert into multi_attribute_data values (8, 1);
insert into multi_attribute_data values (9, 5);
insert into multi_attribute_data values (9, 6);
insert into multi_attribute_data values (10, 4);
insert into multi_attribute_data values (11, 3);
insert into multi_attribute_data values (11, 7);
insert into multi_attribute_data values (11, 5);
insert into multi_attribute_data values (12, 5);
insert into multi_attribute_data values (13, 3);
insert into multi_attribute_data values (15, 8);

