//
//  BLEManager.swift
//  LSCC
//
//  Created by Oliver Jessup on 4/8/2022.
//

import Foundation
import CoreBluetooth

final class BLEManager: NSObject, ObservableObject {
    
    var centralManager: CBCentralManager!
    
    @Published var isSwitchedOn = false
    @Published var loading = true
    
    private var LightsaberPeripheral: CBPeripheral!
    private var txCharacteristic: CBCharacteristic!
    private var rxCharacteristic: CBCharacteristic!
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func startScanning() -> Void {
        centralManager?.scanForPeripherals(withServices: nil, options: nil)
     }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        
        LightsaberPeripheral = peripheral
        LightsaberPeripheral.delegate = self
        
        print("Peripheral Discovered: \(peripheral)")
        print("Peripheral name: \(peripheral.name ?? "No name")")
        print ("Advertisement Data : \(advertisementData)")
        
        if LightsaberPeripheral.identifier.uuidString == "E2E7D8FB-71BB-77D5-40CE-ED704D3876C3" {
            print("Stopped scanning")
            centralManager?.connect(LightsaberPeripheral!, options: nil)
            centralManager?.stopScan()
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        LightsaberPeripheral.discoverServices([CBUUIDs.BLEService_UUID])
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        print("*******************************************************")
        if ((error) != nil) {
            print("Error discovering services: \(error!.localizedDescription)")
            return
        }
        
        guard let services = peripheral.services else {
            return
        }
        
        for service in services {
            peripheral.discoverCharacteristics(nil, for: service)
        }
        print("Discovered Services: \(services)")
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else {
            return
        }
        
        print("Found \(characteristics.count) characteristics.")
        
        for characteristic in characteristics {
            if characteristic.uuid.isEqual(CBUUIDs.BLE_Characteristic_uuid_Rx) {
                rxCharacteristic = characteristic
                
                peripheral.setNotifyValue(true, for: rxCharacteristic!)
                peripheral.readValue(for: characteristic)
                
                print("RX Characteristic: \(rxCharacteristic.uuid)")
            }
            
            if characteristic.uuid.isEqual(CBUUIDs.BLE_Characteristic_uuid_Tx) {
                txCharacteristic = characteristic
                
                print("TX Characteristic: \(txCharacteristic.uuid)")
            }
        }
        loading = false
    }
    
    func disconnectFromDevice() {
        if LightsaberPeripheral != nil {
            centralManager?.cancelPeripheralConnection(LightsaberPeripheral!)
        }
    }
}

extension BLEManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOff:
            print("Powered off")
            isSwitchedOn = false
        case .poweredOn:
            print("Powered on")
            isSwitchedOn = true
            startScanning()
        case .unsupported:
            print("Unsupported")
            isSwitchedOn = false
        case .unauthorized:
            print("Unauthorized")
            isSwitchedOn = false
        case .unknown:
            print("Unknown")
            isSwitchedOn = false
        case .resetting:
            print("Resetting")
            isSwitchedOn = false
        @unknown default:
            print("Error")
            isSwitchedOn = false
        }
    }
}

extension BLEManager: CBPeripheralManagerDelegate {
    func peripheralManagerDidUpdateState(_ peripheral: CBPeripheralManager) {
        switch peripheral.state {
        case .poweredOn:
            print("Peripheral is powered on")
        case .unsupported:
            print("Peripheral is unsupported.")
        case .unauthorized:
            print("Peripheral is unauthorized.")
        case .unknown:
            print("Peripheral unknown")
        case .resetting:
            print("Peripheral resetting")
        case .poweredOff:
            print("Peripheral is powered off.")
        @unknown default:
            print("Error")
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        var characteristicASCIIValue = NSString()
        
        guard characteristic == rxCharacteristic,
              
        let characteristicValue = characteristic.value,
        let ASCIIstring = NSString(data: characteristicValue, encoding: String.Encoding.utf8.rawValue) else {
            return
        }
        
        characteristicASCIIValue = ASCIIstring
        
        print("Value Recieved: \((characteristicASCIIValue as String))")
    }
    
    func writeOutgoingValue(data: String) {
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        
        if let LightsaberPeripheral = LightsaberPeripheral {
            if let txCharacteristic = txCharacteristic {
                LightsaberPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
}

extension BLEManager: CBPeripheralDelegate {
    
}
