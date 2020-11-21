--mysql 
--root/raspberry
--database weather weather/weather
CREATE USER 'weather'@'localhost' IDENTIFIED BY 'weather';
CREATE database weather;
GRANT ALL PRIVILEGES ON weather.* TO 'weather'@'localhost';

create table current_data (
	parameter varchar(32) not null PRIMARY KEY, 
	update_date timestamp not null, 
	param_value NUMERIC(10, 2) not null
);


create table history_data (
	id INT(4) not null AUTO_INCREMENT PRIMARY KEY, 
	parameter varchar(32) not null, 
	update_date timestamp not null, 
	param_value NUMERIC(10, 2) not null
);