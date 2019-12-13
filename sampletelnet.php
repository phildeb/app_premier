<?php
session_start();

/* Allow the script to hang around waiting for connections. */
set_time_limit(10);

/* Turn on implicit output flushing so we see what we're getting * as it comes in. */
ob_implicit_flush();

echo "<h1>Premier asterisk telnet console 2019</h1>";

$address = '127.0.0.1';
$port = 5038;
$manager_user="premier";
$manager_pwd ="premier";


define ("TELNET_ERREUR", 0);
define ("TELNET_OK", 1);


if (1){
$script = new ScriptTelnet;
$script->setHote($address);
$script->setPort($port);
//$script->setPrompt("cet>");
$script->connecter();
$script->ecrire("Action: login\r\n"); 
$script->ecrire("username: ".$manager_user."\r\n"); 
$script->ecrire("Secret: ".$manager_pwd."\r\n"); 
$script->ecrire("\r\n"); 

$cnt=0;
while($script->socket){
	printf("%d)",++$cnt,);
	$ret =  $script->lireJusqua("\r\n");
	
	flush();
	usleep(10000);
	//if ( TELNET_ERREUR == $ret ) exit;
	
}

exit;

$script->deconnecter();
}


Class ScriptTelnet 
{ 
  var $socket  = NULL;
  var $hote = "172.28.16.3";
  var $port = "23";
  var $libelleErreur = "";
  var $codeErreur = "";
  var $prompt = "\\$ ";
  var $log = NULL;  // handle de fichier
  var $repertoireLog= "/tmp/"; 
  var $nomFichierLog = "log-telnet";
  var $test;

  //------------------------------------------------------------------------

  function connecter() 
  {
    $this->socket = fsockopen($this->hote,$this->port); 

    if (! $this->socket)
    { 
      $this->libelleErreur = "Impossible d'etablir la connexion telnet \n";
      return TELNET_ERREUR;
    }

    socket_set_timeout($this->socket,5,0); 
    return TELNET_OK;
  }

  //------------------------------------------------------------------------

  function lireJusqua($chaine) 
  { 
    $NULL = chr(0); 
    $IAC = chr(255);
    $buf = ''; 

    if (! $this->socket)
    {
      $this->libelleErreur = "socket telnet non ouverte";
      return TELNET_ERREUR;
    }
    //printf("\n\n===>lireJusqua[%s]\n",$chaine);

    while (1) 
    { 
      $c = $this->getc(); 
      printf("%s",$c);

      if ($c === false) // plus de caracteres a lire sur la socket 
      {
        if ($this->contientErreur($buf))
          return TELNET_ERREUR;

        $this->libelleErreur = " La chaine attendue : '" . $chaine . "', n'a pas é trouvédans les donné reçs : '" . $buf . "'" ; 
        $this->logger($this->libelleErreur);
        return TELNET_ERREUR;
      }

      if ($c == $NULL || $c == "\\021") 
        continue; 

      if ($c == $IAC)  // Interpreted As Command
      {
        $c = $this->getc(); 

        if ($c != $IAC) // car le 'vrai' caractere 255 est doubléour le differencier du IAC
          if (! $this->negocierOptionTelnet($c))
            return TELNET_ERREUR;
          else
            continue;
      }

      $buf .= $c; 


      if ((substr($buf,strlen($buf)-strlen($chaine))) == $chaine)
      {  
        // on a trouve la chaine attendue

        $this->logger($this->getDernieresLignes($buf));

        if ($this->contientErreur($buf))
          return TELNET_ERREUR;
        else
          return TELNET_OK;        
      }

    }
  }

  //------------------------------------------------------------------------

  function getc()
  {
    return fgetc($this->socket);
  }

  //------------------------------------------------------------------------

  function negocierOptionTelnet($commande)
  {
    // on negocie des options minimales

    $IAC = chr(255); 
    $DONT = chr(254); 
    $DO = chr(253); 
    $WONT = chr(252);      
    $WILL = chr(251); 

    if (($commande == $DO) || ($commande == $DONT)) 
    { 
      $opt = $this->getc();  
      //echo "wont ".ord($opt)."\\n"; 
      fwrite($this->socket,$IAC.$WONT.$opt); 
    } 
    else if (($commande == $WILL) || ($commande == $WONT)) 
    { 
      $opt = fgetc($this->socket);  
      //echo "dont ".ord($opt)."\\n"; 
      fwrite($this->socket,$IAC.$DONT.$opt); 
    } else 
    { 
      $this->libelleErreur = "Erreur : commande inconnue ".ord($commande)."\\n"; 
      return TELNET_ERREUR;
    } 
    return TELNET_OK;
  }

  //------------------------------------------------------------------------

  function ecrire($buffer, $valeurLoggee = "", $ajouterfinLigne = false) 
  {           
    //printf("\n\====>necrire[%s]\n",$buffer);
    if (! $this->socket)
    {
      $this->libelleErreur = "socket non ouverte";
      return TELNET_ERREUR;
    }

    if ($ajouterfinLigne)
      $buffer .= "\\n";

    if (fwrite($this->socket,$buffer) < 0)
    {
      $this->libelleErreur = "erreur d'ecriture sur la socket telnet";
      return TELNET_ERREUR;
    }

    if ($valeurLoggee != "")  // cacher les valeurs confidentielles dans la log (mots de passe...)
      $buffer = $valeurLoggee . "\\n";

    if (! $ajouterfinLigne)  // dans la log (mais pas sur la socket), rajouter tout de meme le caractere de fin de ligne
      $buffer .= "\\n";

    $this->logger("> " .$buffer); 

    return TELNET_OK;
  } 

  //------------------------------------------------------------------------

  function deconnecter() 
  {                    
    if ($this->socket) 
    {
      if (! fclose($this->socket))
      {
        $this->libelleErreur = "erreur a la fermeture de la socket telnet";
        return TELNET_ERREUR;
      }
      $this->socket = NULL; 
    }
    $this->setLog(false,"");

    return TELNET_OK;
  }
  //------------------------------------------------------------------------

  function contientErreur($buf) 
  {   
    $messagesErreurs[] = "nvalid";    // Invalid input, ...
    $messagesErreurs[] = "o specified";     // No specified xxx, ...
    $messagesErreurs[] = "nknown";          // Unknown xxx ...
    $messagesErreurs[] = "o such file or directory"; // sauvegarde dans un repertoire inexistant
    $messagesErreurs[] = "llegal";    // illegal file name, ...

    foreach ($messagesErreurs as $erreur)
    {
      if (strpos ($buf, $erreur) === false)
        continue;

      // une erreur est décté         $this->libelleErreur =  "Un message d'erreur a é déctéans la rénse de l'hôdistant : " . 
      "<BR><BR>" . $this->getDernieresLignes($buf,"<BR>") . "<BR>";

      return true;
    }
  }

  //------------------------------------------------------------------------

  function attendrePrompt()
  {
    return $this->lireJusqua($this->prompt);
  }

  //------------------------------------------------------------------------

  function setPrompt($s) { $this->prompt = $s; return TELNET_OK; }
  //------------------------------------------------------------------------

  function setHote($s) { $this->hote = $s;}

  //------------------------------------------------------------------------

  function setPort($s) { $this->port = $s;}

  //------------------------------------------------------------------------

  function getDerniereErreur() { return $this->libelleErreur; }

  //------------------------------------------------------------------------

  function setLog($activerLog, $traitement) 
  { 
    if ($this->log && $activerLog)  
      return TELNET_OK;

    if ($activerLog)   
    {
      $this->repertoireLog =  MON_HOME . "/log";

      if (! file_exists($this->repertoireLog)) // repertoire mensuel inexistant ?
      {
        if (mkdir($this->repertoireLog, 0700) === false)
        {
          $this->libelleErreur = "Impossible de cr repertoire de log " .  $this->repertoireLog;
          return TELNET_ERREUR;
        }
      }
      global $HTTP_SERVER_VARS;
      $this->nomFichierLog =  date("d") . "_" . 
        date("H:i:s") . "_" . 
        $traitement . "_" . 
        $HTTP_SERVER_VARS["PHP_AUTH_USER"]
        . ".log"; 
      $this->log = fopen($this->repertoireLog . "/" . $this->nomFichierLog,"a");

      if (empty($this->log))
      {
        $this->libelleErreur = "Impossible de cré le fichier de log " . $this->nomFichierLog;
        return TELNET_ERREUR;
      }
      $this->logger("----------------------------------------------\\r\\n");
      $this->logger("Dét de la log de l'utilisateur " . $HTTP_SERVER_VARS["PHP_AUTH_USER"] . 
          ", adresse IP " . $HTTP_SERVER_VARS["REMOTE_ADDR"] . "\\r\\n");
      $this->logger("Connexion telnet sur " . $this->hote . ", port " . $this->port . "\\r\\n");
      $this->logger("Date : " . date("d-m-Y").  "  à" . date("H:i:s") . "\\r\\n");
      $this->logger("Type de traitement effectué " . $traitement . "\\r\\n");
      $this->logger("----------------------------------------------\\r\\n");
      return TELNET_OK;
    }
    else
    {
      if ($this->log) 
      {
        $this->logger("----------------------------------------------\\r\\n");
        $this->logger("Fin de la log\\r\\n");
        fflush($this->log);
        if (! fclose($this->log))
        {
          $this->libelleErreur = "erreur a la fermeture du fichier de log";
          return TELNET_ERREUR;
        }
        $this->log = NULL; 
      }
      return TELNET_OK;
    }
  }

  //------------------------------------------------------------------------

  function logger($s)
  {
    if ($this->log)
      fwrite($this->log, $s);  
  }
  //------------------------------------------------------------------------

  function getDernieresLignes($s, $separateur="\\n")
  {
    // une reponse telnet contient (en principe) en premiere ligne l'echo de la commande utilisateur.
    // cette methode renvoie tout sauf la premiere ligne, afin de ne pas polluer les logs telnet

    $lignes = preg_split("/\\n/",$s);
    $resultat = "";
    $premiereLigne = true;

    while(list($key, $data) = each($lignes))
    {
      if ($premiereLigne)
        $premiereLigne = false;
      else
        if ($data != "")
          $resultat .= $data . $separateur;
    }
    $resultat == substr($resultat,strlen($resultat)-1); // enlever le dernier caractere de fin de ligne

    return $resultat;
  }

  //------------------------------------------------------------------------


}   //    Fin de la classe                    


?>

