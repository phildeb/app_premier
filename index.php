<?php
session_start();

/* Allow the script to hang around waiting for connections. */
set_time_limit(10);

/* Turn on implicit output flushing so we see what we're getting * as it comes in. */
ob_implicit_flush();

require("astclass.php");

$address = '127.0.0.1';
$port = 5038;
$manager_user="premier";
$manager_pwd ="premier";
$uploads_dir = "/var/lib/asterisk/sounds/";

echo "<h1>Premier asterisk studio 2019</h1>";

if ( isset($_GET["btn_clear_session"]) ){	session_unset();}

if ( isset($_GET["basename"])) $_SESSION['basename'] = $_GET["basename"];

printf("<fieldset><legend>Actions</legend>");
printf("<form action='%s'  method='GET'>" ,$_SERVER['PHP_SELF']);
//printf("<td><input type=submit name='btn_show_peers' value='show peers'>");
printf("<td><input type=submit name='btn_status' value='show status'>");
printf("<td><input type=submit name='btn_clear_session' value='clear session'>");
printf("</form>");
printf("</fieldset>");

if ( isset($_GET["btn_status"]) )
{
    $myast = new AstMan;
    if ( true==$myast->Login("localhost",$manager_user,$manager_pwd))
    {
        $qry="Action: Status\r\n";        
        $qry.="\r\n";

        $reponse = $myast->Query($qry);
        printf("<hr>command:%s",$qry);
        printf("<hr>response :%s",$reponse);
        $reponse = $myast->Gets();
        $arr= explode("\r",$reponse);
        foreach($arr as $key=>$val){
            $kkvv= explode(":",$val);
            //printf("<hr>K=%s V=[%s] (%d)",$key,$val,count($kkvv));
            if ( count($kkvv)>=2 ){				
                //printf("TEST [%s]<hr>",trim($kkvv[0]) ) ;
                if ( trim($kkvv[0])==="Channel" ) {
					$sipchan = trim($kkvv[1]);
					$_SESSION['channel']=$sipchan;
					//printf("<hr>FOUND [%s]",$_SESSION['channel']);
				}				
			}            
		}
	}
}

if ( isset($_GET["btn_play_in_call"]) )
{
    $strName = $_GET["filename"];
    $filenamenoextension = $_GET["filenamenoextension"];
    $fwavname = preg_replace("[^A-Za-z0-9_-]", "",$strName);

    if ( !function_exists('socket_create') )
    {
        printf("<h1>erreur version php socket_create inexistante</h1>");
    }else{
        $myast = new AstMan;
        if ( true==$myast->Login("localhost",$manager_user,$manager_pwd))
        {            
            $qry="";
            $qry.="Action: PremierPlayForeground\r\n";
            $qry.=sprintf("channel: %s\r\n" , $_SESSION['channel']);       
            $qry.=sprintf("filename: %s\r\n" , $_SESSION['basename']);     
            $qry.="\r\n";

            $reponse = $myast->Query($qry);
            printf("<hr>command:%s</h1>",$qry);
            printf("<hr>response :%s</h1>",$reponse);
            $myast->Logout();
        }else{
            printf("<h1>erreor connection asterisk manager</h1>");
        }

    }

}

echo "<br>";

if(1){
	
	if ($handle = opendir($uploads_dir)) {
		while (false !== ($file = readdir($handle))) {			
				if ($file != "." && $file != "..") {
						$fullpathname = $uploads_dir."/".$file;
						$arr = explode(".", $file);
						$extension = end($arr);
						if ( $extension === "wav"  )
						{
								//printf("%s %d octets<br>",$fullpathname , filesize($fullpathname));
								$tabGV[$fullpathname] = filesize($fullpathname);
								//dbg($fullpathname);
						}
				}
		}
		closedir($handle);
	}	

	ksort($tabGV);

		$N=0;
		if ( isset($tabGV) ) {
				printf("<table class='stat'>");
				printf("<th>No");
				printf("<th>Prompt");
				printf("<th>Size (Ko)");
				//if (is_admin()) printf("<th>Supprimer");
				foreach($tabGV as $key=>$val){

						//printf("<option %s value='%s'>%s (%d octets)</option>",$selected, $key,$key,$val);	
						$path_parts = pathinfo($key);

						//print_r($path_parts);continue;
						$fpathnoextension = substr($key,0,strlen($key)-1-strlen($path_parts["extension"]));//path_parts["dirname"]."/".$path_parts["filename"];			
						//$fpathnoextension_selected = $fpathnoextension;
						//$relative_dir = str_replace("/var/spool/asterisk","http://".$_SERVER['SERVER_ADDR'],$fpathnoextension);				
						$relative_dir = str_replace("/usr/share/asterisk","http://".$_SERVER['SERVER_ADDR'] ."/premier/",$fpathnoextension);				
						$extension = $path_parts["extension"];
						$basename = $path_parts["basename"];

						if ( strtolower($extension) !== "wav") continue;

						printf("<form action='%s'  method='GET'>" ,$_SERVER['PHP_SELF']);
						
						printf("<tr>");
						printf("<td>%s",++$N);
						
						if ( strlen($_SESSION['channel']) ) 
						printf("<td><input type=submit name='btn_play_in_call' value='Play in %s'>", $_SESSION['channel']);
						
						
						printf("<input type='hidden' name='newdir' value='%s'>", $uploads_dir);
						printf("<input type='hidden' name='filename' value='%s'>", $key);									
						printf("<input type='hidden' name='filenamenoextension' value='%s'>", $fpathnoextension);									
						printf("<input type='hidden' name='basename' value='%s'>", $basename);									
						printf("<input type='hidden' name='predec' value='%s'>", $relative_dir.".wav");
						printf("<td>");
						
						printf("%s", $basename);
						
						printf("<td>%.1f Ko",$val/1024);

						printf("</form>");
				}
				printf("</table>");
		}
		printf("</fieldset>");
}


if ( isset(	$_SESSION) && count($_SESSION) ) {
	printf("<fieldset><legend>SESSION variables</legend>");
	print_r($_SESSION);
	printf("</fieldset>");
	printf("<br>");
}

?>
