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
?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Weather</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
</head>
<body>
<table border="0">
	<tr>
		<td>
			<h1>Larin home weather service
		</td>
	</tr>
	<tr>
		<td valign="top">
			<table border="1">
			<?php
			while ($row = mysqli_fetch_row($result)) {
			  echo "<tr>";
			  echo "<td>$row[0]</td>";
			  echo "<td>$row[1]</td>";
			  echo "<td>$row[2]</td>";
			  echo "</tr>";
			}
			?>			
			</table>
		</td>
		<td>
		</td>
	</tr>
</table>

</body>
</html>

<?php
	mysqli_close($connection);
?>
