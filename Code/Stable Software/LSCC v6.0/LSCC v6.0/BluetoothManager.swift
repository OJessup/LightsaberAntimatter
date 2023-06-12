//
//  BluetoothManager.swift
//  LSCC v6.0
//
//  Created by Oliver Jessup on 31/3/2023.
//

import Foundation
import CoreBluetooth

class BluetoothManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    
    var centralManager: CBCentralManager!
    @Published var peripherals = [CBPeripheral]()
    @Published var connectedPeripheral: CBPeripheral?
    
    @Published var humFile: String = "Deep"
    @Published var IRFile: String = "Default"
    @Published var shutdownOnDrop: Bool = false
    @Published var powerSetting: Double = 150.0
    
    @Published var status: String = "Running"
    @Published var battery: Double = 100.0
    @Published var isCharging: Bool = false
    
    //var txCharacteristic: CBCharacteristic!
    //var rxCharacteristic: CBCharacteristic!
    let txCharacteristic = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
    let rxCharacteristic = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
    let serviceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func scanForPeripherals() {
        centralManager.scanForPeripherals(withServices: nil, options: nil)
    }
    
    func connect(to peripheral: CBPeripheral) {
        centralManager.connect(peripheral, options: nil)
    }
    
    func disconnect(from peripheral: CBPeripheral) {
        centralManager.cancelPeripheralConnection(peripheral)
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            scanForPeripherals()
        } else {
            print("Bluetooth is not available")
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if (peripheral.name != nil) {
            if !peripherals.contains(peripheral) {
                peripherals.append(peripheral)
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connectedPeripheral = peripheral
        connectedPeripheral?.delegate = self
        connectedPeripheral?.discoverServices([serviceUUID])
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        connectedPeripheral = nil
        peripherals.removeAll(where: { $0.identifier == peripheral.identifier })
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let services = peripheral.services {
            for service in services {
                peripheral.discoverCharacteristics(nil, for: service)
            }
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let characteristics = service.characteristics {
            for characteristic in characteristics {
                if characteristic.uuid/*.isEqual(CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"))*/== rxCharacteristic {
                    //rxCharacteristic = characteristic
                    
                    peripheral.setNotifyValue(true, for: characteristic)
                    peripheral.readValue(for: characteristic)
                }
                
                /*if characteristic.uuid.isEqual(CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")) {
                    txCharacteristic = characteristic
                }*/
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if (characteristic.uuid == rxCharacteristic && error == nil) {
            //Recieved data
            if let value = characteristic.value {
                let stringValue = String(data: value, encoding: .utf8)
                let stringValueComponents = stringValue?.components(separatedBy: ",")
                
                //Set data
                IRFile = stringValueComponents![0]
                humFile = stringValueComponents![1]
                powerSetting = Double(stringValueComponents![3])!
                battery = Double(stringValueComponents![4])!
                status = stringValueComponents![5]
                if(stringValueComponents![2] == "0") {
                    shutdownOnDrop = false
                } else {
                    shutdownOnDrop = true
                }
                if(status == "Charging") {
                    isCharging = true
                } else {
                    isCharging = false
                }
                
            }
        }
    }
    
    func sendData(data: Data) {
        if let characteristic = connectedPeripheral?.services?.first?.characteristics?.first(where: { $0.uuid == txCharacteristic }) {
            connectedPeripheral!.writeValue(data, for: characteristic, type: .withResponse)
        }
    }
    
}
