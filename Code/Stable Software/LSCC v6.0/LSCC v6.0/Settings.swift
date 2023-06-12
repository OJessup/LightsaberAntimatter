//
//  Settings.swift
//  LSCC v6.0
//
//  Created by Oliver Jessup on 31/3/2023.
//

import SwiftUI

struct Settings: View {
    
    @ObservedObject var bluetoothManager: BluetoothManager
    
    @State var counter: Int = 20
    
    var body: some View {
        VStack {
            Button {
                var temp: String = ""
                if (bluetoothManager.humFile == "Deep") {
                    temp = "VeryDeep"
                } else if (bluetoothManager.humFile == "VeryDeep") {
                    temp = "VeryDeepDistored"
                } else if (bluetoothManager.humFile == "VeryDeepDistored") {
                    temp = "DeepAlternativeDistorted"
                } else if (bluetoothManager.humFile == "DeepAlternativeDistorted") {
                    temp = "Light"
                } else if (bluetoothManager.humFile == "Light") {
                    temp = "HighLight"
                } else if (bluetoothManager.humFile == "HighLight") {
                    temp = "SuperLight"
                } else if (bluetoothManager.humFile == "SuperLight") {
                    temp = "Deep"
                }
                bluetoothManager.sendData(data: (("=" + temp) as NSString).data(using: String.Encoding.utf8.rawValue)!)
            } label: {
                Text("Hum Sound: \(bluetoothManager.humFile)")
                    .frame(maxWidth: .infinity)
                    .foregroundColor(.white)
                    .padding()
                    .background(Color.accentColor.cornerRadius(20))
            }

            Button {
                var temp: String = ""
                if (bluetoothManager.IRFile == "Default") {
                    temp = "SemiDeep"
                } else if (bluetoothManager.IRFile == "SemiDeep") {
                    temp = "Deep"
                } else if (bluetoothManager.IRFile == "Deep") {
                    temp = "Alternative"
                } else if (bluetoothManager.IRFile == "Alternative") {
                    temp = "Quick"
                } else if (bluetoothManager.IRFile == "Quick") {
                    temp = "Default"
                }
                bluetoothManager.sendData(data: (("&" + temp) as NSString).data(using: String.Encoding.utf8.rawValue)!)
            } label: {
                Text("Ignition Sound: \(bluetoothManager.IRFile)")
                    .frame(maxWidth: .infinity)
                    .foregroundColor(.white)
                    .padding()
                    .background(Color.accentColor.cornerRadius(20))
            }

            //Toggle("Turn off when dropped", isOn: $bluetoothManager.shutdownOnDrop)
            //    .onChange(of: bluetoothManager.shutdownOnDrop) { value in
            //        bluetoothManager.sendData(data: ("+" as NSString).data(using: String.Encoding.utf8.rawValue)!)
            //    }
            
            HStack {
                Text("Power Setting")
                Slider(value: Binding(get: {
                    self.bluetoothManager.powerSetting
                }, set: { (newVal) in
                    self.bluetoothManager.powerSetting = newVal
                    self.sendPowerData()
                }), in: 0...255)
                .tint(.orange)
            }
        }
        .padding()
    }
    
    func sendPowerData() {
        if(counter == 0) {
            let dataToSend = "_\(UInt8(bluetoothManager.powerSetting))"
            bluetoothManager.sendData(data: (dataToSend.description as NSString).data(using: String.Encoding.utf8.rawValue)!)
            counter = 20
        } else {
            counter -= 1
        }
    }
    
    func uint8ToHexString(_ value: UInt8) -> String {
        return String(format: "%02X", value)
    }
    
    func sendHex(hexString: String) {
        let data = Data(Array<UInt8>(hexString.hexadecimal!))
        bluetoothManager.sendData(data: data)
    }
}

extension String {
    var hexadecimal: [UInt8]? {
        var startIndex = self.startIndex
        return (0..<count/2).compactMap { _ in
            guard let endIndex = index(startIndex, offsetBy: 2, limitedBy: endIndex) else { return nil }
            defer { startIndex = endIndex }
            return UInt8(self[startIndex..<endIndex], radix: 16)
        }
    }
}
