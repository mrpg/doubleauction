<?php
$n = 1000;

for ($i = 0; $i < $n; $i++) {
	if (mt_rand(0,1) == 0) {
		echo "B ";
	}
	else {
		echo "S ";
	}

	echo mt_rand(1,100)." ".mt_rand(1,100)." 0 0\n";
}
?>
