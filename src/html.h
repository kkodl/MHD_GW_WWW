// HTML web page to handle input fields
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html lang = "cs">
<head>
  <meta http-equiv="Content-type" content="text/html; charset=utf-8">
  <title>ESP32 Golemio Gateway Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  
  <style>
    html {font-family: Arial;}
    h2{color:green;}
    h3{color:green}
    p {color:blue;}
  </style>
  
  <script>
    function submitMessage() {
    alert("Saved value to ESP file");
    setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script>
    
</head>
  
<body>

<h3>Obecná nastavení:</h3>

  <form action="/get" target="hidden-form">
    Golemio token: <br> <input type="text" style="background-color: lightblue; color: black;" name="GolemToken" size="30" value = %GolemToken%>
    <input type="submit" value="OK" onclick="submitMessage()">
    </form>
<br>
  <form action="/get" target="hidden-form">
    Živý Obraz import-key: <br> <input type="text" style="background-color: lightblue; color: black;"  name="ZivyObrazToken" size="30" value = %ZivyObrazToken%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
<br>
  <form action="/get" target="hidden-form">
    Report Interval: <br> <input type="text" style="background-color: lightblue; color: black;" name="Report" size="30" value = %Report%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
<p>------------------------------------------------------------------------------------</p>
<h3>Zastávka #1: %Zastavka1TextName%</h3>
    <form action="/get" target="hidden-form">
    Kód zastávky: <br> <input type="text" style="background-color: lightblue; color: black;" name="Stop1Name" size="19" value = %Stop1Name%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>
  <form action="/get" target="hidden-form">
    Počet odjezdů (1 až 5): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop1Trains" size="16" min="1" max="5" value = %Stop1Trains%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>  
  <form action="/get" target="hidden-form">
    Čas příchodu (minut):<br> <input type="number" style="background-color: lightblue; color: black;" name="Stop1Walk" size="16"min="0" max="30" value = %Stop1Walk%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>

<p>------------------------------------------------------------------------------------</p>
<h3>Zastávka #2: %Zastavka2TextName%</h3>
    <form action="/get" target="hidden-form">
    Kód zastávky: <br> <input type="text" style="background-color: lightblue; color: black;" name="Stop2Name" size="19" value = %Stop2Name%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>
    <form action="/get" target="hidden-form">
    Počet odjezdů (1 až 5): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop2Trains" size="16" min="1" max="5" value = %Stop2Trains%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>  
    <form action="/get" target="hidden-form">
    Čas příchodu (minut): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop2Walk" size="16"min="0" max="30" value = %Stop2Walk%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>

<p>------------------------------------------------------------------------------------</p>
<h3>Zastávka #3: %Zastavka3TextName%</h3>
    <form action="/get" target="hidden-form">
    Kód zastávky: <br> <input type="text" style="background-color: lightblue; color: black;" name="Stop3Name" size="19" value = %Stop3Name%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>
    <form action="/get" target="hidden-form">
    Počet odjezdů (1 až 5): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop3Trains" size="16" min="1" max="5" value = %Stop3Trains%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>  
    <form action="/get" target="hidden-form">
    Čas příchodu (minut): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop3Walk" size="16"min="0" max="30" value = %Stop3Walk%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>

<p>------------------------------------------------------------------------------------</p>
<h3>Zastávka #4: %Zastavka4TextName%</h3>
    <form action="/get" target="hidden-form">
    Kód zastávky: <br> <input type="text" style="background-color: lightblue; color: black;" name="Stop4Name" size="19" value = %Stop4Name%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>
    <form action="/get" target="hidden-form">
    Počet odjezdů (1 až 5): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop4Trains" size="16" min="1" max="5" value = %Stop4Trains%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>  
    <form action="/get" target="hidden-form">
    Čas příchodu (minut): <br> <input type="number" style="background-color: lightblue; color: black;" name="Stop4Walk" size="16" min="0" max="30"  value = %Stop4Walk%>
    <input type="submit" value="OK" onclick="submitMessage()">
  </form>

<p>------------------------------------------------------------------------------------</p>
  <h3>Gateway status:</h3>
    Gateway status ---> %GoFlag%
    <br>
    Transmit data to Zivy Obraz ---> %TxFlag%
  <br>
  </form>
  <br>
  <form action="/get" target="hidden-form">
  <label for="GoFlag">Gateway status: </label>
  <select name="GoFlag" id="GoFlag">
    <option value="stop">Stop</option>
    <option value="run">Run</option>
  </select>
   <input type="submit" value="OK" onclick="submitMessage()">
  </form>
  <br>
  <form action="/get" target="hidden-form">
  <label for="TxFlag">Transmit  to  ZO: </label>
  <select name="TxFlag" id="TxFlag">
    <option value="no">No</option>
    <option value="yes">Yes</option>
  </select>
   <input type="submit" value="OK" onclick="submitMessage()">
  </form>

<p>------------------------------------------------------------------------------------</p>
  <iframe style="display:none" name="hidden-form"></iframe>

</body>

</html>

)rawliteral";
