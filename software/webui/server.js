var express = require('express');
var app = express();

var bodyParser = require('body-parser');
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies


app.use(express.static(__dirname + '/html'));
var status_code = 200;
var random_wait = 100;

app.get("/wifi", function(req,res) {
	
	var json_response;
	
	//wifi status and networks
	json_response = {"status": {"Hostname":"big_seven_0001","SSID":"big_seven_0001", "IP": "192.168.4.1"}, "networks": [{"ssid":"5a105e8b9d40e1329780d62ea2265d8a", "auth":1, "quality": 90}, {"ssid":"ad0234829205b9033196ba818f7a872b", "auth":0, "quality": 50}, {"ssid":"8ad8757baa8564dc136c1e07507f4a98", "auth":1, "quality": 85}]};
	//wifi status no networks
	//json_response = {"status": {"Hostname":"big_seven_0001","SSID":"big_seven_0001", "IP": "192.168.4.1"}, "networks": []};
	
	//insert some random wait
	setTimeout( function () {
		//simulate .fail on front end side via http code
		res.status(status_code);
		res.json(json_response);
	}, random_wait * Math.random());
	
	console.log("[GET] /wifi");
});

app.get("/time", function(req, res) {
	
	var json_response = {"ntp_server": "eu.ntp.pool.org", "time_zone": '+7200', "time_dst": 'no'};
	
	//insert some random wait
	setTimeout( function () {
		//simulate .fail on front end side via http code
		res.status(status_code);
		res.json(json_response);
	}, random_wait * Math.random());
	
	console.log("[GET] /time");
});

/*
 *	each post request should at least return 
 *  error: false/true 
 *  message: text - which should contain relevant information to the post request
 */

app.post("/wifi", function(req, res) {
	
	var ssid = req.body.ssid;
	var pass = req.body.pass;
	var json_response;
	
	//fail response
	if( pass.length == 0 || ssid.length == 0)
		json_response = {"error": true, "message": "Password can't be empty!"};
	else {
		//success response
		json_response = {"error": false, "message": "http://localhost:8080/#wifi"};
	}
	
	//insert some random wait
	setTimeout( function () {
		//simulate .fail on front end side via http code
		res.status(status_code);
		res.json(json_response);
	}, random_wait * Math.random());
	
	console.log("[POST] /wifi - SSID: " + ssid + ", password: " + pass);
});

app.post("/time", function(req,res) {
	
	//POST variables from request
	var ntp_server = req.body.ntp_server;
	var time_zone = req.body.time_zone;
	var time_dst = req.body.time_dst;
	var json_response;
	
	//fail response
	json_response = {"error": true, "message": "can't save config file on flash"};
	//success response, not sure if sending data back is required
	json_response = {"error": false, "message": "", "ntp_server": ntp_server, "time_zone": time_zone, "time_dst": time_dst};
	
	//insert some random wait
	setTimeout( function () {
		//simulate .fail on front end side via http code
		res.status(status_code);
		res.json(json_response);
	}, random_wait * Math.random());
	console.log("[POST] /time ntp server: "+ntp_server+", time zone: "+time_zone+", dst: "+time_dst+"")
});

app.listen(8080);
console.log('Application server up and running on port 8080');