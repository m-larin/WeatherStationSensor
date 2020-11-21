<?php 
	error_reporting(E_ALL);

	$host = 'localhost';
	$user = 'weather';
	$pass = 'weather';
	$db   = 'weather';

	$connection = mysqli_connect($host, $user, $pass, $db) 
		or die("Could not connect: " . mysqli_error());

	$sql = "select * from current_data";

	$result = mysqli_query($connection, $sql) 
		or die("Query failed: " . mysql_error());
	
	$temperature = 100;
	while ($row = mysqli_fetch_row($result)) {
		if ($row[0] == "bmp085_pressure"){
			$pressure = $row[2];
		}else if ($row[0] == "radio_17_temperature" || $row[0] == "radio_18_temperature"){
			if ($temperature > $row[2]){
				$temperature = $row[2];
			}
		}else if ($row[0] == "radio_18_humidity"){
			$humidity = $row[2];
		}
		
		$change_date = date_create($row[1]);
		$today = date_create("now");
		$diff = date_diff($today, $change_date);
		if ($diff->y || $diff->m || $diff->d || $diff->h > 1){
			$sensor_problem = true;
		}
	}
	mysqli_close($connection);
		
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Weather</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
	body{
		background: #d7f6fc;
	}
	
	.problem{
		color: red;
	}
	
	.panel{
		background: #f799c4;
		color: white;
		width: 200px;
		height: 200px;
		text-align: center;
		margin:10px auto;
		font-weight: bold;
		font-family: Arial;
		border-radius: 10px;
		box-shadow: 0 14px 28px rgba(0,0,0,0.25), 0 10px 10px rgba(0,0,0,0.22);
	}

	.panel_body{
		width: 200px;
		height: 170px;
		vertical-align:middle; 
		display:table-cell;
		font-size: xx-large;
	}

	
	.panel_header{
		color: white;
	}
	
	
</style>
</head>
<body>
	<?php
		if ($sensor_problem){
	?>
		<div class="problem">
			Ошибка датчиков
		</div>
	<?php
	    }
	?>
	<div class="panel">
		<div class="panel_header">Температура</div>
		<div class="panel_body"><?php echo "$temperature &deg;" ?></div>
	</div>
	<div class="panel">
		<div class="panel_header">Давление</div>
		<div class="panel_body"><?php echo "$pressure мм" ?></div>
	</div>
	<div class="panel">
		<div class="panel_header">Влажность</div>
		<div class="panel_body"><?php echo "$humidity %"?></div>
	</div>
</body>
</html>

