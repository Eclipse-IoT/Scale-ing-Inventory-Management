/*******************************************************************************
 * Copyright (c) 2020 Gregory Ivo
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 * 
 * Contributors:
 *    Gregory Ivo
 *******************************************************************************/

const iofog = require('@iofog/nodejs-sdk');

//Default values, will be changed when app is deployed though ioFOG
let iofogConfig = {
  mqttServer: '192.168.0.14',
  port: '1883',
  username: 'cedalo',
  password: 'M7wgN0BhUIffVWkBPWyr1l7nb',
  name: 'factory1',
};


//Functional Vars

var countCollection = {
  devices: []
}

//End of Functional Vars


//mqtt
var mqtt = require('mqtt')
var localMQTTClient = mqtt.connect('mqtt://localhost')
var remoteMQTTClint = mqtt.connect('mqtt://' + iofogConfig.username + ':' + iofogConfig.password + '@' + iofogConfig.mqttServer + '/')
var topicToRead = "cedalo/scaleOut"; //Topic Local MQTT looks for scale messages in, and remote MQTT will write too
var topicToNotify = "cedalo/presence"; //Topic for status updates


//Express
const express = require('express');
const { response } = require('express');
var path = require('path');
const { config } = require('process');
const { connect } = require('http2');
const { userInfo } = require('os');
const PORT = 8080;
const HOST = '0.0.0.0';
const app = express();
var log = "starting of log:<br>"

//Express config
function webLog(arg) {
  //clear log at arbitrary size
  if (log.length > 5000) {
    log = "starting of log:<br>"
  }
  log += "<br>" + arg + "<br>"
}

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname + '/index.html'));
});

app.get('/iofogconfig', (req, res) => {
  res.send(iofogConfig);
});

app.get('/log', (req, res) => {
  res.send(log);
});

app.get('/count', (req, res) => {
  res.send(countCollection);
});

app.get('/reconnectMQTT', (req, res) => {
  localMQTTClient = mqtt.connect('mqtt://localhost')
  remoteMQTTClint = mqtt.connect('mqtt://' + iofogConfig.username + ':' + iofogConfig.password + '@' + iofogConfig.mqttServer + '/')
  res.send("attempting");
});



app.get('/status', (req, res) => {
  var msg = ('localMQTTClient conneced: ' + localMQTTClient.connected + "<br>");
  msg += ('localMQTTClient reconnecting: ' + localMQTTClient.reconnecting + "<br>");
  msg += ('localMQTTClient disconnected: ' + localMQTTClient.disconnected + "<br><br>");
  if (remoteMQTTClint != null) {
    msg += ('MQTT 2 conneced: ' + remoteMQTTClint.connected + "<br>");
    msg += ('MQTT 2 reconnecting: ' + remoteMQTTClint.reconnecting + "<br>");
    msg += ('MQTT 2 disconnected: ' + remoteMQTTClint.disconnected + "<br><br>");
    res.send(msg);
  }
});
//End Express Config


//LOCAL MQTT SERVER CONFIG
localMQTTClient.on('connect', function () {
  localMQTTClient.subscribe(topicToRead, function (err) {
    if (!err) {
      localMQTTClient.publish(topicToNotify, 'Hello mqtt')
      console.log("Successfully subscribed to: " + topicToRead);
      webLog("Successfully subscribed to: " + topicToRead);
    } else {
      console.log("Failed to subscribe to: " + topicToRead);
      webLog("Failed to subscribe to: " + topicToRead);
    }
  })
})

//recive MQTT MESSAGE
localMQTTClient.on('message', function (topic, message) {

  try {
    //try to parse message as JSON
    var JsonMSG = JSON.parse(message.toString());     //convert message to JSON
    
    //Check to see if msg is from compatable scale by seeing if it has a id feild
    if (JsonMSG.id) {
      console.log(JsonMSG);
      if (remoteMQTTClint.connected) {
        console.log("attmpting to deliver mqtt");
        
        //Adding more data to the JSON msg
        var paddedJSON = {
          id: JsonMSG.id,
          count: JsonMSG.cnt,
          location: iofogConfig.name,
          part: JsonMSG.pt,
          time: Date.now()
        }
        console.log(paddedJSON);
        var tempTopic = topicToRead + "/" + iofogConfig.name + "/" + JsonMSG.id;
        console.log(tempTopic)

        //Can remove this line if one does not every message being sent seperatly
        remoteMQTTClint.publish(tempTopic, JSON.stringify(paddedJSON));
      }

      //creating collection of counts for easy streamsheets graph making
      var doesnotcontain = true;
      countCollection.devices.forEach((device) => {
        if (device.id == JsonMSG.id) {
          device.cnt = JsonMSG.cnt;
          doesnotcontain = false;
        }
      })
      if (doesnotcontain) {
        countCollection.devices.push({
          id: JsonMSG.id,
          cnt: JsonMSG.cnt
        })
      }

      var colectionArray = {}
      colectionArray.name = iofogConfig.name

      countCollection.devices.forEach((device) => {
        colectionArray[device.id] = device.cnt;
      });
      
      //publish collection
      remoteMQTTClint.publish((topicToRead + "/" + iofogConfig.name), JSON.stringify(colectionArray));
    }

  } catch (err) {
    console.log("Not a JSON");
    webLog("not a JSON");
  }

})

//MQTT 2 (REMOTE)
remoteMQTTClint.on('connect', function () {
  console.log('REMOTE: connect');
  webLog('REMOTE: connected');
  remoteMQTTClint.publish(topicToNotify, 'Remote connected!')
})


//IOFOG CONFIG
function updateConfig() {
  iofog.getConfig({
    onNewConfig: newConfig => {

      if (newConfig != null && newConfig != iofogConfig) {
        console.log("changing ioFOG Config");
        webLog("changing ioFOG Config");
        iofogConfig = newConfig;
        webLog("connecting to MQTT2");
        //assuming config has been updated, start secconday MQTT Server
        remoteMQTTClint = mqtt.connect('mqtt://' + iofogConfig.username + ':' + iofogConfig.password + '@' + iofogConfig.mqttServer + '/')
      }
      console.log(iofogConfig.toString());
    },
    onBadRequest: err => console.error('updateConfig failed: ', err),
    onError: err => console.error('updateConfig failed: ', err)
  });
}

//GET iofog config
function main() {
  updateConfig();
}
//End IOFOG CONFIG





app.listen(PORT, HOST);
console.log(`Web-server Running on http://${HOST}:${PORT}`);

webLog("attempting to connect to ioFOG")
iofog.init('iofog', 54321, null, main);


