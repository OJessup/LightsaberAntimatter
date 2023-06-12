//
//  ContentView.swift
//  LSCC v6.0
//
//  Created by Oliver Jessup on 30/3/2023.
//

import SwiftUI

struct ContentView: View {
    @State var colour: Color = .primary
    
    @StateObject var bluetoothManager = BluetoothManager()
    
    var body: some View {
        VStack {
            if let connectedPeripheral = bluetoothManager.connectedPeripheral {
                Text("Connected to \(connectedPeripheral.name ?? "Unknown")")
                
                Button(action: {
                    bluetoothManager.disconnect(from: connectedPeripheral)
                }) {
                    Text("Disconnect")
                        .padding(5)
                }
            } else {
                Text("Bluetooth Devices")
                
                ScrollView {
                    ForEach(bluetoothManager.peripherals, id: \.self) { peripheral in
                        Button(action: {
                            bluetoothManager.connect(to: peripheral)
                        }) {
                            Text(peripheral.name ?? "Unknown")
                                .padding(5)
                                .frame(maxWidth: .infinity)
                        }
                    }
                }
                .frame(maxHeight: 100)
            }
            
            ZStack {
                ColourWheel(selectedColour: $colour, bluetoothManager: bluetoothManager)
                
                Circle()
                    .frame(width: 50, height: 50)
                    .foregroundColor(colour)
                    .offset(x: 140, y: 140)
            }
            .padding()
            
            Settings(bluetoothManager: bluetoothManager)
            
            HStack {
                Text("Status: \(bluetoothManager.status)")
                
                if (!bluetoothManager.isCharging) {
                    Text("Battery: \(Int(bluetoothManager.battery))%")
                }
            }
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
