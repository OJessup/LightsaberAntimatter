//
//  BLEcontroller.swift
//  LSCC v1.0
//
//  Created by Oliver Jessup on 8/8/2022.
//

import CoreBluetooth

protocol BLEcontrollerProtocol {
    func state(state: BLEcontroller.State)
    func value(data: Data)
}

final class BLEcontroller: NSObject {
    static let shared = BLEcontroller()
    var delegate: BLEcontrollerProtocol?
    
    var current: CBPeripheral?
    var state: State = .unknown { didSet { delegate?.state(state: state) } }
    
    private var manager: CBCentralManager?
    private var readCharacteristic: CBCharacteristic?
    private var writeCharacteristic: CBCharacteristic?
    private var notifyCharacteristic: CBCharacteristic?
    
    private override init() {
        super.init()
        manager = CBCentralManager(delegate: self, queue: .none)
        manager?.delegate = self
    }
    
    func connect(_ peripheral: CBPeripheral) {
        if current != nil {
            guard let current = current else { return }
            manager?.cancelPeripheralConnection(current)
            manager?.connect(peripheral, options: nil)
        } else { manager?.connect(peripheral, options: nil) }
    }
    
    func disconnect() {
        guard let current = current else { return }
        manager?.cancelPeripheralConnection(current)
    }
    
    func startScanning() {
        manager?.scanForPeripherals(withServices: nil, options: nil)
    }
    func stopScanning() {
        manager?.stopScan()
    }
    
    func send(_ value: [UInt8]) {
        guard let characteristic = writeCharacteristic else { return }
        current?.writeValue(Data(value), for: characteristic, type: .withResponse)
    }
    
    enum State { case unknown, resetting, unsupported, unauthorized, poweredOff, poweredOn, error, connected, disconnected }
}

extension BLEcontroller: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch manager?.state {
        case .unknown: state = .unknown
        case .resetting: state = .resetting
        case .unsupported: state = .unsupported
        case .unauthorized: state = .unauthorized
        case .poweredOff: state = .poweredOff
        case .poweredOn:
            state = .poweredOn
            startScanning()
        default: state = .error
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        
        print("Peripheral Discovered: \(peripheral)")
        print("Peripheral name: \(peripheral.name ?? "No name")")
        print ("Advertisement Data : \(advertisementData)")
        
        if peripheral.identifier.uuidString == "E2E7D8FB-71BB-77D5-40CE-ED704D3876C3" {
            stopScanning()
            manager?.connect(peripheral, options: nil)
        }
        
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        current = peripheral
        state = .connected
        peripheral.delegate = self
        peripheral.discoverServices(nil)
    }
}

extension BLEcontroller: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        for service in services {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else { return }
        for characteristic in characteristics {
            switch characteristic.properties {
            case .read:
                readCharacteristic = characteristic
            case .write:
                writeCharacteristic = characteristic
            case .notify:
                notifyCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
            case .indicate: break //print("indicate")
            case .broadcast: break //print("broadcast")
            default: break
            }
        }
    }
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor descriptor: CBDescriptor, error: Error?) { }
    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) { }
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard let value = characteristic.value else { return }
        delegate?.value(data: value)
    }
}
