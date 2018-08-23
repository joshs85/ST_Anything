/**
 *  Child Switch
 *
 *  Copyright 2017 Daniel Ogorchock
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 *  Change History:
 *
 *    Date        Who            What
 *    ----        ---            ----
 *    2017-09-05  Joshua Spain  Original Creation
 *
 * 
 */
metadata {
	definition (name: "Child Blind", namespace: "ogiewon", author: "Dan Ogorchock") {
		capability "Window Shade"
        capability "Switch"
		capability "Relay Switch"
		capability "Actuator"
		capability "Sensor"
        command "my"

        attribute "lastUpdated", "String"
	}

	tiles(scale: 2) {
    
        // standard tile with actions
     		multiAttributeTile(name:"blind", type: "generic", width: 3, height: 4, canChangeIcon: true){
			tileAttribute ("blind", key: "PRIMARY_CONTROL") {
				attributeState "closed", label: '${name}', action: "windowShade.open", icon: "st.switches.switch.off", backgroundColor: "#ffffff", nextState:"opening"
				attributeState "open", label: '${name}', action: "windowShade.close", icon: "st.switches.switch.on", backgroundColor: "#00A0DC", nextState:"closing"
				attributeState "opening", label:'${name}', action:"windowShade.close", icon:"st.switches.switch.on", backgroundColor:"#00A0DC", nextState:"closing"
				attributeState "closing", label:'${name}', action:"windowShade.open", icon:"st.switches.switch.off", backgroundColor:"#ffffff", nextState:"opening"
			}
            tileAttribute("device.lastUpdated", key: "SECONDARY_CONTROL") {
                attributeState("default", label:'    Last updated ${currentValue}',icon: "st.Health & Wellness.health9")
            }
		}
        
       	standardTile("open", "generic", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
			state "open", label:'open', action:"windowShade.open"
		}
       	standardTile("close", "generic", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
			state "closed", label:'close', action:"windowShade.close"
		}
       	standardTile("my", "generic", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
			state "default", label:'my', action:"my"
		}
        main "blind"
		details(["blind", "open", "close", "my"])
	}
}

	preferences {
		input "SecondsToOpen", "int",
			title: "Time it takes in seconds to open the blinds.",
			description: "Enter the number of seconds it takes to open the shades.",
			defaultValue: "10" as Integer,
			required: false,
			displayDuringSetup: true
            
		input "SecondsToClose", "integer",
			title: "Time it takes in seconds to close the blinds.",
			description: "Enter the number of seconds it takes to close the shades.",
			defaultValue: "10" as Integer,
			required: false,
			displayDuringSetup: true
            }

void open() {
	sendData("open")
}

void close() {
	sendData("close")
}

void my() {
	sendData("stop")
}

void on(){
	open()
}

void off(){
	close()
}

def sendData(String value) {
    def name = device.deviceNetworkId.split("-")[-1]
    parent.sendData("${name} ${value}")
}

def parse(String description) {
    log.debug "parse(${description}) called"
	def parts = description.split(" ")
    def name  = parts.length>0?parts[0].trim():null
    def value = parts.length>1?parts[1].trim():null
    if (name && value) {
        // Update device
        sendEvent(name: name, value: value)
        // Update lastUpdated date and time
        def nowDay = new Date().format("MMM dd", location.timeZone)
        def nowTime = new Date().format("h:mm a", location.timeZone)
        sendEvent(name: "lastUpdated", value: nowDay + " at " + nowTime, displayed: false)
    }
    else {
    	log.debug "Missing either name or value.  Cannot parse!"
    }
}

def installed() {
}