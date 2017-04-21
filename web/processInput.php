<?php

$input = $_POST['simulationInput'];

$file = 'modsim.in';
file_put_contents($file, $input);


shell_exec("modsim.exe");

$output = file_get_contents('modsim.out');
?>


<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Simulation Output</title>
</head>
<body>
    <div class="simulation-output">
        <pre><?= $output ?></pre>
    </div>
</body>
</html>