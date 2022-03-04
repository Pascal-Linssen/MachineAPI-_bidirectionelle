function Buttonessorage() {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "essorage", true);
  xhttp.send();
}

function Buttonjeton() {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "jeton", true);
  xhttp.send();
}

function Buttonreset() {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "reset", true);
  xhttp.send();
}

function Buttonstop() {
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "stop", true);
  xhttp.send();
}

setInterval(function getData() {
  var xhttp = new XMLHttpRequest();

  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("valeurJeton").innerHTML = this.responseText;
    }
  };

  xhttp.open("GET", "NombreJeton", true);
  xhttp.send();
}, 2000);

setInterval(function getData() {
  var xhttp = new XMLHttpRequest();

  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("Temperature1").innerHTML = this.responseText;
    }
  };

  xhttp.open("GET", "Temperature", true);
  xhttp.send();
}, 2000);

setInterval(function getData() {
  var xhttp = new XMLHttpRequest();

  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ValeurEtat").innerHTML = this.responseText;
    }
  };

  xhttp.open("GET", "NombreEtat", true);
  xhttp.send();
}, 2000);
