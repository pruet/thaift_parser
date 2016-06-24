CREATE DATABASE IF NOT EXISTS test;

USE test;

SELECT 'simple test (s1-s8)' as '';

DROP TABLE IF EXISTS t;

CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=InnoDB CHARACTER SET utf8 COLLATE utf8_unicode_ci;

insert into t(id, c) values(1, 'อกุศลวิตก (unwholesome thought; akusala vitaka)'), (2, 'อกุศล (unwholesome; akusala)'), (3, 'วิตก (thought; vitaka)');

select id as 's1 expect 1,3' from t where match c against ('วิตก' in boolean mode); #expect 1,3
select id as 's2 expect 1,2' from t where match c against ('อกุศล' in boolean mode); #expect 1,2
select id as 's3 epxect 1'   from t where match c against ('+วิตก +akusala' in boolean mode); #expect 1
select id as 's4 expect 1'   from t where match c against ('+วิตก +อกุศล' in boolean mode); #expect 1
select id as 's5 expect 1,3' from t where match c against ('+วิตก akusala' in boolean mode); #expect 1,3
select id as 's6 expect 1,3' from t where match c against ('+วิตก อกุศล' in boolean mode); #expect 1,3
select id as 's7 expect 2'   from t where match c against ('-วิตก akusala' in boolean mode); #expect 2
select id as 's8 expect 2'   from t where match c against ('-วิตก +อกุศล' in boolean mode); #expect 2


############# Longer words 

SELECT 'long test (l1-l4)' as '';

DROP TABLE IF EXISTS t;

CREATE TABLE t (id INT, c VARCHAR(255), FULLTEXT (c) WITH PARSER thaift_parser) ENGINE=InnoDB CHARACTER SET utf8 COLLATE utf8_unicode_ci;

insert into t(id, c) values(1, 'ละอกุศลวิตก'), (2, 'ละกามวิตก'), (3, 'ซึ่งวิหิงสาวิตก');
select id as '' from t where match c against ('ละ -อกุศลวิตก' in boolean mode); #expect empty
select id as 'l2 expect 1' from t where match c against ('+อกุศลวิตก' in boolean mode); #expect 1
select id as 'l3 expect 1,2,3' from t where match c against ('อกุศลวิตก' in boolean mode); #expect 1,2,3
select id as 'l4 expect 3' from t where match c against ('+วิหิงสา +ซึ่ง' in boolean mode); #expect 3