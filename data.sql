create table Students (
    zid integer primary key,
    name PersonName not null
);

insert into Students(zid, name) values
(1001, 'Smith.John');