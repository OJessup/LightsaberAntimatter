//
//  ColourWheel.swift
//  LSCC v6.0
//
//  Created by Oliver Jessup on 30/3/2023.
//

import SwiftUI

struct ColourWheel: View {
    @Binding var selectedColour: Color
    @ObservedObject var bluetoothManager: BluetoothManager
    
    let brightness: Double = 1.0
    let radius: CGFloat = 150
    var diameter: CGFloat {radius*2.0}
    
    @State var sends: Int = 10
    
    @State var tapAngle: Double? = nil
    
    var body: some View {
        Circle()
            .fill(
                AngularGradient(gradient: Gradient(colors: [.red, .yellow, .green, .blue, .purple, .red]), center: .center)
            )
            .frame(width: diameter, height: diameter)
            .overlay (
                Circle()
                    .fill(
                        RadialGradient(gradient: Gradient(colors: [.white, .white.opacity(0.000001)]), center: .center, startRadius: 0,endRadius: radius)
                    )
            )
            .gesture (
                DragGesture(minimumDistance: 0)
                    .onChanged { value in
                        let vector = CGVector(dx: value.location.x - radius, dy: value.location.y - radius)
                        let radians = atan2(vector.dy, vector.dx)
                        var degrees = radians * 180 / .pi
                        if (degrees < 0) {degrees += 360}
                        var distance = sqrt(vector.dx * vector.dx + vector.dy * vector.dy)
                        if (distance > 150) {distance = 150}
                        selectedColour = Color(hue: Double(degrees / 360), saturation: distance/150, brightness: brightness)
                        let theStringColour: String = selectedColour.description
                        if(sends == 0) {
                            print(theStringColour)
                            bluetoothManager.sendData(data: (selectedColour.description as NSString).data(using: String.Encoding.utf8.rawValue)!)
                            sends = 10
                        } else {
                            sends -= 1
                        }
                    }
            )
    }
}
