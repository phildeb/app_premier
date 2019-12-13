<?php

printf("AMI event loop\n");

$manager_user="premier";
$manager_pwd ="premier";

$errstr="";

if (1){

    ob_start(); 
    $socket = @fsockopen("127.0.0.1","5038", $errno, $errstr, 1);
    if (!$socket){
        echo "$errstr ($errno)\n";
    }else{
        fputs($socket, "action: login\r\n");
        fputs($socket, "username: ".$manager_user."\r\n");
        fputs($socket, "secret: ".$manager_pwd."\r\n\r\n");
        fputs($socket, "action: Waitevent\r\n");
        //fputs($socket, "Eventmask: call\r\n");
        $wrets=fgets($socket,128);

        $line=0;
        {
            while(($buffer = fgets($socket,128)) !== false)
            { 
            $line++;
            //$p1=strpos($buffer,"Newchannel");      //if($p1 > 0)    echo "Calling ID =";//echo substr($buffer,$p1+4,4);
            flush();ob_flush();
            printf("%d [%s]\n",$line, trim($buffer));        
            }     
        }
    }
}
?>
