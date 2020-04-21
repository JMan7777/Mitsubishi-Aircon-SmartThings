/*
  Mitsubishi Aircon SmartThings Device Handler
 
  This device handler works only in combination with the
  https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings
  Arduino sketch installed on an ESP32
 
  Copyright (c) 2020 Markus Jessl.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
*/
 
import groovy.json.JsonSlurper

metadata {
	definition (
		name: "Mitsubishi Aircon Device Handler",
		namespace: "JMan7777",
		author: "Markus Jessl") 
        {
            capability "Switch"
            capability "Refresh"
            capability "Temperature Measurement"
            command "tempUp"
            command "tempDown"
            command "fan"
            command "vSwing"
            command "hSwing"
            command "mode"
        }

	preferences {
		input name: "DeviceIP", type: "text", title: "Device IP Address", description: "ESP32 ip address", defaultValue: "192.168.1.123", required: true, displayDuringSetup: true
        input name: "DevicePort", type: "text", title: "Device Port", description: "ESP32 webserver port", defaultValue: "80", required: true, displayDuringSetup: true
        input name: "DeviceTemperatureUnit", type: "text", title: "Temp Unit", description: "Temperature Unit (C or F)", defaultValue: "C", required: true, displayDuringSetup: true
	}

	simulator {
	}

	tiles(scale: 2) {
		multiAttributeTile(name:"thermostatFull", type:"thermostat", width:6, height:4) {
			tileAttribute("device.temperature", key: "PRIMARY_CONTROL") {
				attributeState("temperature", label:'${currentValue}', defaultState: true, 
                backgroundColors:[
                	//Celsius
					[value: 16, color: "#153591"],
					[value: 19, color: "#1e9cbb"],
					[value: 21, color: "#90d2a7"],
					[value: 23, color: "#44b621"],
					[value: 26, color: "#f1d801"],
					[value: 29, color: "#d04e00"],
					[value: 31, color: "#bc2323"],
                    //Fahrenheit
					[value: 38, color: "#153591"],
					[value: 44, color: "#1e9cbb"],
					[value: 59, color: "#90d2a7"],
					[value: 74, color: "#44b621"],
					[value: 84, color: "#f1d801"],
					[value: 95, color: "#d04e00"],
					[value: 96, color: "#bc2323"]                    
				])
			}
			tileAttribute("device.temperature", key: "VALUE_CONTROL") {
				attributeState("VALUE_UP", action: "tempUp")
				attributeState("VALUE_DOWN", action: "tempDown")
			}
		}
        standardTile("switch", "device.switch", width: 2, height: 2, decoration: "flat", canChangeIcon: true) {
            state "off", label:'Off', action:"switch.on", icon:"st.thermostat.ac.air-conditioning", backgroundColor:"#ffffff", nextState:"turningOn"
            state "on", label:'On', action:"switch.off", icon:"st.thermostat.ac.air-conditioning", backgroundColor:"#00a0dc", nextState:"turningOff"
            state "turningOn", label:'Turning on', action:"switch.off", icon:"st.thermostat.ac.air-conditioning", backgroundColor:"#dcdcdc", nextState: "turningOff"
            state "turningOff", label:'Turning off', action:"switch.on", icon:"st.thermostat.ac.air-conditioning", backgroundColor:"#dcdcdc", nextState: "turningOn"
            state "?", label:'Unknown', icon:"st.thermostat.ac.air-conditioning", backgroundColor:"#ffffff"
        }
        
        valueTile("roomTemperature", "device.roomTemperature", width: 2, height: 2, decoration: "flat") {
			state("roomTemperature", label:'${currentValue}', icon:"st.alarm.temperature.normal")
            state("?", label:'Unknown', icon:"st.alarm.temperature.normal", backgroundColor:"#ffffff")
      	}
        
		standardTile("fan", "device.fan", width: 2, height: 2, canChangeIcon: true, decoration: "flat") {
			state "1", label:'1', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "2", label:'2', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "3", label:'3', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "4", label:'4', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "QUIET", label:'Quiet', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "AUTO", label:'Auto', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "CHANGING", label:'Changing', action:"fan", icon:"st.Appliances.appliances11", backgroundColor:"#dcdcdc", nextState: "AUTO"
            state "?", label:'Unknown', icon:"st.Appliances.appliances11", backgroundColor:"#ffffff"
		}
		standardTile("vSwing", "device.vSwing", width: 2, height: 2, canChangeIcon: true, decoration: "flat") {
			state "1", label:'1', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "2", label:'2', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "3", label:'3', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "4", label:'4', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "5", label:'5', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "SWING", label:'Swing', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "AUTO", label:'Auto', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "CHANGING", label:'Changing', action:"vSwing", icon:"st.vents.vent-open", backgroundColor:"#dcdcdc", nextState: "AUTO"
            state "?", label:'Unknown', icon:"st.vents.vent-open", backgroundColor:"#ffffff"
		}
        
		standardTile("hSwing", "device.hSwing", width: 2, height: 2, canChangeIcon: true, decoration: "flat") {
			state "<<", label:'<<', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "<", label:'<', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "|", label:'|', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#ffffff", nextState: "CHANGING"
            state ">", label:'>', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#ffffff", nextState: "CHANGING"
            state ">>", label:'>>', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "SWING", label:'Swing', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "AUTO", label:'Auto', action:"hSwing", icon:"st.motion.motion.inactive" , backgroundColor:"#ffffff", nextState: "CHANGING"
            state "CHANGING", label:'Changing', action:"hSwing", icon:"st.motion.motion.inactive", backgroundColor:"#dcdcdc", nextState: "AUTO"
            state "?", label:'Unknown', icon:"st.motion.motion.inactive", backgroundColor:"#ffffff"
		}
                        
        standardTile("mode", "device.mode", width: 2, height: 2, canChangeIcon: true, decoration: "flat") {
			state "COOL", label:'Cool', action:"mode", icon:"st.Weather.weather7", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "HEAT", label:'Heat', action:"mode", icon:"st.Weather.weather14", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "FAN", label:'Fan', action:"mode", icon:"st.Weather.weather1", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "DRY", label:'Dry', action:"mode", icon:"st.Weather.weather10", backgroundColor:"#ffffff", nextState: "CHANGING"
            state "AUTO", label:'Auto', action:"mode", icon:"st.Weather.weather11", backgroundColor:"#ffffff", nextState: "CHANGING"
			state "CHANGING", label:'Changing', action:"mode", icon:"st.Weather.weather11", backgroundColor:"#dcdcdc", nextState: "AUTO"
            state "?", label:'Unknown', icon:"st.Weather.weather11", backgroundColor:"#ffffff"
		}

		standardTile("refresh", "device.refresh", height: 2, width: 2, decoration: "flat") {
        	state "default", label:'Refresh', action:"refresh", icon:"st.secondary.refresh", backgroundColor:"#ffffff", nextState:"refreshing"
            state "refreshing", label:'Refreshing', icon:"st.secondary.refresh", backgroundColor:"#dcdcdc", nextState: "default"            
    	}
        
		main("switch")
		details([
			"thermostatFull", 
            "switch",
            "refresh",
            "roomTemperature",
			"fan",
            "vSwing",
            "hSwing",
            "mode"
		])
	}
}

def int fahrenheitToCelcius(int f) {
   	log.debug "Converting ${f}F  to C"
	//T(°C) = (T(°F) - 32) / 1.8
    def float c = ((f - 32) / 1.8)
    return c.round(0)
}

def int celsiusToFahrenheit(int c) {
   	log.debug "Converting ${c}C to F"
	//T(°F) = T(°C) × 1.8 + 32 
    def float f = ((c * 1.8) + 32)
    return f.round(0)
}

def installed() {
	initialize()
}

def updated() {
	initialize()
}

def initialize() {
	unschedule(autoRefresh)
    runEvery1Minute(autoRefresh)
}

def autoRefresh() {

	def String refresh = device.currentValue("refresh")
    
    if (refresh.equals("default"))
    {
        log.debug "Auto - Refresh"
        sendEvent(name: "refresh", value: "refreshing", isStateChange: true)
        runCmd("GET_SETTINGS", "")    
    }
    else
    {
    	log.debug "Auto - Refresh skipped, already refreshing"
    }
}

def refresh() {
	log.debug "Refresh"
	runCmd("GET_SETTINGS", "")    
}

def on() {
	log.debug "Power on"
	runCmd("POWER", "ON")
}

def off() {
	log.debug "Power off"
	runCmd("POWER", "OFF")
}

def tempUp() {
	log.debug "Temp up"
	def int temp = device.currentValue("temperature") 
   
    def String tempUnit = "${DeviceTemperatureUnit}"
    log.debug "Temp Unit = ${tempUnit}"
    if (tempUnit.equals("F"))
    {
    	temp = fahrenheitToCelcius(temp)
        temp = temp + 1
    }
    else
    {
        log.debug "No temp conversion needed."
    }

    if (temp <= 31)
    {	
		runCmd("TEMP", "${temp}")
    }
    else
    {
    	log.error "Temp out of range"
    }
}

def tempDown() {
	log.debug "Temp down"
	def int temp = device.currentValue("temperature")
    
    def String tempUnit = "${DeviceTemperatureUnit}"
    log.debug "Temp Unit = ${tempUnit}"
    if (tempUnit.equals("F"))
    {
    	temp = fahrenheitToCelcius(temp)
        temp = temp - 1
    }
    else
    {
        log.debug "No temp conversion needed."
    }

	if (temp >= 16)
    {	
		runCmd("TEMP", "${temp}")
    }
    else
    {
    	log.error "Temp out of range"
    }
}

def fan() {
	log.debug "Fan change"
	def String fan = device.currentValue("fan")
    def String fanNew = "${fan}"
    switch (fan) {
        case "1":
        	fanNew = "2"
            break
        case "2":
        	fanNew = "3"
            break
        case "3":
        	fanNew = "4"
            break
        case "4":
        	fanNew = "AUTO"
            break
        case "AUTO":
        	fanNew = "QUIET"
            break
        case "QUIET":
        	fanNew = "1"
            break
        default:
        	log.error "Unknown fan setting: ${fan}"
            break
	}
    if (!fanNew.equals(fan))
    {
    	runCmd("FAN", "${fanNew}")
    }
    else
    {
		log.debug "Fan settings unchanged."
    }
}

def vSwing() {
	log.debug "vSwing change"
	def String vSwing = device.currentValue("vSwing")
    def String vSwingNew = "${vSwing}"
    switch (vSwing) {
        case "1":
        	vSwingNew = "2"
            break
        case "2":
        	vSwingNew = "3"
            break
        case "3":
        	vSwingNew = "4"
            break
        case "4":
        	vSwingNew = "5"
            break
        case "5":
        	vSwingNew = "AUTO"
            break
        case "AUTO":
        	vSwingNew = "SWING"
            break
        case "SWING":
        	vSwingNew = "1"
            break
        default:
        	log.error "Unknown vSwing setting: ${vSwing}"
            break
	}
    if (!vSwingNew.equals(vSwing))
    {
    	runCmd("V_SWING", "${vSwingNew}")
    }
    else
    {
		log.debug "vSwing settings unchanged."
    }
}

def hSwing() {
	log.debug "hSwing change"
	def String hSwing = device.currentValue("hSwing")
    def String hSwingNew = "${hSwing}"
    switch (hSwing) {
        case "<<":
        	hSwingNew = "<"
            break
        case "<":
        	hSwingNew = "|"
            break
        case "|":
        	hSwingNew = ">"
            break
        case ">":
        	hSwingNew = ">>"
            break
        case ">>":
        	hSwingNew = "SWING"
            break
        case "SWING":
        	hSwingNew = "AUTO"
            break
        case "AUTO":
        	hSwingNew = "<<"
            break
        default:
        	log.error "Unknown hSwing setting: ${hSwing}"
            break
	}
    if (!hSwingNew.equals(hSwing))
    {
    	runCmd("H_SWING", "${hSwingNew}")
    }
    else
    {
		log.debug "vSwing settings unchanged."
    }
}

def mode() {
	log.debug "Mode change"
	def String mode = device.currentValue("mode")
    def String modeNew = "${mode}"
    switch (mode) {
        case "COOL":
        	modeNew = "HEAT"
            break
        case "HEAT":
        	modeNew = "FAN"
            break
        case "FAN":
        	modeNew = "DRY"
            break
        case "DRY":
        	modeNew = "AUTO"
            break
        case "AUTO":
        	modeNew = "COOL"
            break
        default:
        	log.error "Unknown mode setting: ${mode}"
            break
	}
    if (!modeNew.equals(mode))
    {
    	runCmd("MODE", "${modeNew}")
    }
    else
    {
		log.debug "Mode settings unchanged."
    }
}

def runCmd(String varCommand, String value) { 

	def path = '/ChangeAirconSettings'
    def body = ''
	def method = "POST"

	switch (varCommand) {
        case "POWER":
            body = '{"' + varCommand + '":"' + value + '"}';
        	log.debug "POST body is: $body"
            break
        case "MODE":
        	body = '{"' + varCommand + '":"' + value + '"}';
        	log.debug "POST body is: $body"
            break
        case "TEMP":
        	body = '{"' + varCommand + '":"' + value + '"}';
        	log.debug "POST body is: $body"
            break
        case "FAN":
        	body = '{"' + varCommand + '":"' + value + '"}';
			log.debug "POST body is: $body"        
            break
        case "V_SWING":
        	body = '{"' + varCommand + '":"' + value + '"}';
        	log.debug "POST body is: $body"        
            break
        case "H_SWING":
        	body = '{"' + varCommand + '":"' + value + '"}';
			log.debug "POST body is: $body"        
            break
        case "GET_SETTINGS":
        	path = "/GetAirconSettings"
            method = "GET"
            break
        default:
        	log.error "Wrong command = $varCommand"
            break
    }


	def host = DeviceIP
	def hosthex = convertIPtoHex(host).toUpperCase()
	def porthex = convertPortToHex(DevicePort).toUpperCase()
	device.deviceNetworkId = "$hosthex:$porthex"

	log.debug "Device ID hex = $device.deviceNetworkId"

	def headers = [:] 
	headers.put("HOST", "$host:$DevicePort")
	headers.put("Content-Type", "application/json")

	log.debug "ESP32 IP:Port = $headers"

	
	try {
    	runIn(15, commandTimeOut)
		def hubAction = new physicalgraph.device.HubAction(
			method: method,
			path: path,
			body: body,
			headers: headers
			)
		hubAction.options = [outputMsgToS3:false]
		log.debug hubAction
		sendHubCommand(hubAction)
	}
	catch (Exception e) {
		log.error "Hit Exception $e on $hubAction"
        resetRefreshTile()
	}
}

def resetRefreshTile(){
    sendEvent(name: "refresh", value: "default", isStateChange: true)
}

def commandTimeOut(){
	log.error "Command to ESP32 timed out."
    resetRefreshTile();
}

def parse(String description) {
	def whichTile = ''	
   	def map = [:]
	def retResult = []
	def descMap = parseDescriptionAsMap(description)
	def bodyReturned = ' '
	def headersReturned = ' '
	if (descMap["body"]) { 
    		bodyReturned = new String(descMap["body"].decodeBase64())
            log.debug "Response:  $bodyReturned"
            }
	if (descMap["headers"]) { headersReturned = new String(descMap["headers"].decodeBase64()) }
	
	if (bodyReturned != ' ') {
        def data = parseJson(bodyReturned)
        
        def String roomTemp = "${data."ROOM_TEMP"}"
        def String temp = "${data."TEMP"}"
        if (roomTemp.equals("?"))
        {
        	roomTemp = "0"
        }
        if (temp.equals("?"))
        {
        	temp = "0"
        }
        
        def String tempUnit = "${DeviceTemperatureUnit}"
        log.debug "Temp Unit = ${tempUnit}"
        
        if (tempUnit.equals("F"))
        {
        	roomTemp = "${celsiusToFahrenheit(roomTemp.toInteger())}"
            temp = "${celsiusToFahrenheit(temp.toInteger())}"
        }
        else
        {
            log.debug "No temp conversion needed."
        }
        
    	sendEvent(name: "roomTemperature", value: roomTemp, isStateChange: true)
        sendEvent(name: "temperature", value: temp, isStateChange: true)
        sendEvent(name: "mode", value: "${data."MODE"}", isStateChange: true)
        sendEvent(name: "fan", value: "${data."FAN"}", isStateChange: true)
        sendEvent(name: "vSwing", value: "${data."V_SWING"}", isStateChange: true)
        sendEvent(name: "hSwing", value: "${data."H_SWING"}", isStateChange: true)
        sendEvent(name: "switch", value: "${data."POWER"}".toLowerCase(), isStateChange: true)
	}
    
    log.debug "Response processed"
    
  	unschedule(commandTimeOut)
    
    resetRefreshTile()
}

def parseDescriptionAsMap(description) {
	description.split(",").inject([:]) { map, param ->
	def nameAndValue = param.split(":")
	map += [(nameAndValue[0].trim()):nameAndValue[1].trim()]
	}
}

private String convertIPtoHex(ipAddress) {
	String hex = ipAddress.tokenize( '.' ).collect {  String.format( '%02x', it.toInteger() ) }.join()
	return hex
}

private String convertPortToHex(port) {
	String hexport = port.toString().format( '%04x', port.toInteger() )
	return hexport
}

private Integer convertHexToInt(hex) {
	Integer.parseInt(hex,16)
}

private String convertHexToIP(hex) {
	[convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
}

private getHostAddress() {
	def parts = device.deviceNetworkId.split(":")
	def ip = convertHexToIP(parts[0])
	def port = convertHexToInt(parts[1])
	return ip + ":" + port
}
