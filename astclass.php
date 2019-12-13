<?php

class AstMan
{

 var $socket;
 var $error;
 var $wrets;

 function AstMan()
 {
   $this->socket = FALSE;
   $this->error = "";
 }

 function Login($host="localhost", $username="admin", $password="amp111"){

   $this->socket = @fsockopen("127.0.0.1","5038", $errno, $errstr, 1);
   if (!$this->socket) {
     $this->error =  "Could not connect - $errstr ($errno)";
     return FALSE;
   }else{
     stream_set_timeout($this->socket, 1);
     //socket_set_nonblock($this->socket);

     $wrets = $this->Query("Action: Login\r\nUserName: $username\r\nSecret: $password\r\nEvents: off\r\n\r\n");

     if (strpos($wrets, "Message: Authentication accepted") != FALSE){
       return true;
     }else{
    $this->error = "Could not login - Authentication failed";
       fclose($this->socket);
       $this->socket = FALSE;
    return FALSE;
     }
   }
 }

 function Logout(){
   if ($this->socket){
     fputs($this->socket, "Action: Logoff\r\n\r\n");
     $wrets="";
     while (!feof($this->socket)) {
       $wrets .= fread($this->socket, 8192);
     }
     fclose($this->socket);
     $this->socket = "FALSE";
   }
  return;
 }

 function Gets(){
     $info = stream_get_meta_data($this->socket);
     //print_r($info);
     $line="";
     if(0){
         $nb=$info['unread_bytes'];
         $str=fgets($this->socket, $nb);
         $line .= $str;
     }
     //while ($line != "\r\n"  || strlen($line) < 100);
     if (!feof($this->socket)) {
         $line .= fread($this->socket,8192);
     }
     return $line;
 }


 function Query($query){
   $wrets = "";

   if ($this->socket === FALSE)
     return FALSE;

   fputs($this->socket, $query);
   do
   {
     $line = fgets($this->socket, 4096);
     $wrets .= $line;
     $info = stream_get_meta_data($this->socket);
   }
   //while ($line != "\r\n" && $infotimed_out>'timed_out' == false );
   while ($line != "\r\n" );
   return $wrets;
 }

 function GetError(){
   return $this->error;
 }

 function GetDB($family, $key){
   $value = "";

   $wrets = $this->Query("Action: Command\r\nCommand: database get $family $key\r\n\r\n");

   if ($wrets){
     $value_start = strpos($wrets, "Value: ") + 7;
     $value_stop = strpos($wrets, "\n", $value_start);
    if ($value_start > 8){
       $value = substr($wrets, $value_start, $value_stop - $value_start);
     }
  }
   return $value;
 }

 function PutDB($family, $key, $value){
   $wrets = $this->Query("Action: Command\r\nCommand: database put $family $key $value\r\n\r\n");

  if (strpos($wrets, "Updated database successfully") != FALSE){
  return TRUE;
   }
   $this->error =  "Could not updated database";
   return FALSE;
 }

 function DelDB($family, $key){
   $wrets = $this->Query("Action: Command\r\nCommand: database del $family $key\r\n\r\n");

  if (strpos($wrets, "Database entry removed.") != FALSE){
  return TRUE;
   }
   $this->error =  "Database entry does not exist";
   return FALSE;
 }


 function GetFamilyDB($family){
   $wrets = $this->Query("Action: Command\r\nCommand: database show $family\r\n\r\n");
   if ($wrets){
     $value_start = strpos($wrets, "Response: Follows\r\n") + 19;
     $value_stop = strpos($wrets, "--END COMMAND--\r\n", $value_start);
    if ($value_start > 18){
       $wrets = substr($wrets, $value_start, $value_stop - $value_start);
     }
     $lines = explode("\n", $wrets);
     foreach($lines as $line){
       if (strlen($line) > 4){
         $value_start = strpos($line, ": ") + 2;
         $value_stop = strpos($line, " ", $value_start);
         $key = trim(substr($line, strlen($family) + 2, strpos($line, " ") - strlen($family) + 2));
         //$value$key = trim(substr($line, $value_start));
		 }
     }
     return $value;
  }
   return FALSE;
 }
};


?>
