//
//  ViewController.swift
//  Lightsaber control panel
//
//  Created by Oliver Jessup on 28/7/2022.
//

import UIKit
import CoreBluetooth

class ViewController: UIViewController {
    
    var centralManager: CBCentralManager!
    
    private var LightsaberPeripheral: CBPeripheral!
    private var txCharacteristic: CBCharacteristic!
    private var rxCharacteristic: CBCharacteristic!

    override func viewDidLoad() {
        super.viewDidLoad()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func startScanning() -> Void {
        //Start scanning
        centralManager?.scanForPeripherals(withServices: nil, options: nil)
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        
        LightsaberPeripheral = peripheral
        LightsaberPeripheral.delegate = self
        
        print("Peripheral Discovered: \(peripheral)")
        print("Peripheral name: \(peripheral.name ?? "No name")")
        print ("Advertisement Data : \(advertisementData)")
        
        //Check if current bluetooth device is the lightsaber
        if LightsaberPeripheral.identifier.uuidString == "E2E7D8FB-71BB-77D5-40CE-ED704D3876C3" {
            print("Stopped scanning")
            centralManager?.connect(LightsaberPeripheral!, options: nil)
            centralManager.stopScan()
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        //Discover lightsaber services
        LightsaberPeripheral.discoverServices([CBUUIDs.BLEService_UUID])
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        //Found lightsaber services
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
        //Find characteristics
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
    }
    
    func disconnectFromDevice() {
        //Disconnect function
        if LightsaberPeripheral != nil {
            centralManager?.cancelPeripheralConnection(LightsaberPeripheral!)
        }
    }
    
    @IBAction func SendData(_ sender: Any) {
        writeOutgoingValue(data: "hi")
    }
    
}

extension ViewController: CBCentralManagerDelegate {
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        
        switch central.state {
        case .poweredOff:
            print("Powered off")
        case .poweredOn:
            print("Powered on")
            startScanning()
        case .unsupported:
            print("Unsupported")
        case .unauthorized:
            print("Unauthorized")
        case .unknown:
            print("Unknown")
        case .resetting:
            print("Resetting")
        @unknown default:
            print("Error")
        }
    }
}

extension ViewController: CBPeripheralManagerDelegate {
    
    func peripheralManagerDidUpdateState(_ peripheral: CBPeripheralManager){
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
        //Read data
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
        //Write data
        let valueString = (data as NSString).data(using: String.Encoding.utf8.rawValue)
        
        if let LightsaberPeripheral = LightsaberPeripheral {
            if let txCharacteristic = txCharacteristic {
                LightsaberPeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
}

extension ViewController: UIColorPickerViewControllerDelegate {
    
    func colorPickerViewControllerDidFinish(_ viewController: UIColorPickerViewController) {
        self.view.backgroundColor = viewController.selectedColor
    }
    
    func colorPickerViewControllerDidSelectColor(_ viewController: UIColorPickerViewController) {
        self.view.backgroundColor = viewController.selectedColor
        let rgb = String(format: "%02X", viewController.selectedColor.rgb()!)
        writeOutgoingValue(data: rgb)
        
    }
}

extension UIColor {
     func rgb() -> Int? {
         var fRed : CGFloat = 0
         var fGreen : CGFloat = 0
         var fBlue : CGFloat = 0
         var fAlpha: CGFloat = 0
         if self.getRed(&fRed, green: &fGreen, blue: &fBlue, alpha: &fAlpha) {
             let iRed = Int(fRed * 255.0)
             let iGreen = Int(fGreen * 255.0)
             let iBlue = Int(fBlue * 255.0)
             //let iAlpha = Int(fAlpha * 255.0)
             //  (Bits 24-31 are alpha, 16-23 are red, 8-15 are green, 0-7 are blue).
             let rgb = /*(iAlpha << 24) + */(iRed << 16) + (iGreen << 8) + iBlue
             return rgb
         } else {
             // Could not extract RGBA components:
             return nil
         }
     }
 }

extension ViewController: CBPeripheralDelegate {
     
}

