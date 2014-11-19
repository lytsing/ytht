CREATE TABLE spam (
  Id bigint unsigned auto_increment not null,
  User char(20) NOT NULL default '',
  Time int NOT NULL default '0',
  Title char(255) NOT NULL default '',
  Magic int not null default '0',
  Filename char(20) not null default '',
  Sender char(255) not null default '',
  Mailtype int not null,
  Feedback int not null default '0',
  KEY Id (Id, Magic,User),
  KEY User (User, Filename)
) TYPE=MyISAM COMMENT='spam!';
