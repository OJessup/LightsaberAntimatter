//
//  ContentView.swift
//  LSCC
//
//  Created by Oliver Jessup on 3/8/2022.
//

import SwiftUI

struct ContentView: View {
    
    @EnvironmentObject var appState: AppState
    
    @ObservedObject var bleManager = BLEManager()
    
    @State private var location: CGPoint?
    @State private var StartLocation: CGPoint?
    @State private var SelectedColour = Color.white
    private var I12Origin: CGPoint = CGPoint(x: 195.0, y: 415.75)
    
    let yOffset: CGFloat = -180
    let radius: CGFloat = 150
    var diameter: CGFloat {
        radius * 2
    }
    
    var body: some View {
        ZStack {
            Circle()
                .fill(
                    AngularGradient(gradient: Gradient(colors: [
                        Color(hue: 1.0, saturation: 1, brightness: 0.9),
                        Color(hue: 0.9, saturation: 1, brightness: 0.9),
                        Color(hue: 0.8, saturation: 1, brightness: 0.9),
                        Color(hue: 0.7, saturation: 1, brightness: 0.9),
                        Color(hue: 0.6, saturation: 1, brightness: 0.9),
                        Color(hue: 0.5, saturation: 1, brightness: 0.9),
                        Color(hue: 0.4, saturation: 1, brightness: 0.9),
                        Color(hue: 0.3, saturation: 1, brightness: 0.9),
                        Color(hue: 0.2, saturation: 1, brightness: 0.9),
                        Color(hue: 0.1, saturation: 1, brightness: 0.9),
                        Color(hue: 0.0, saturation: 1, brightness: 0.9)
                    ]), center: .center)
                )
                .frame(width: diameter, height: diameter)
                .overlay(
                    Circle()
                        .fill(
                            RadialGradient(gradient: Gradient(colors: [
                                Color.white,
                                Color.white.opacity(0.000001)
                            ]), center: .center, startRadius: 0, endRadius: radius)
                        )
                )
                .position(x: I12Origin.x, y: I12Origin.y + yOffset)
                .shadow(color: Color.black.opacity(0.2), radius: 6, y: 8)
            
            if StartLocation != nil {
                Circle()
                    .frame(width: 50, height: 50)
                    .position(location!)
                    .foregroundColor(Color.white)
                    .shadow(color: Color.black.opacity(0.2), radius: 6, y: 8)
            }
            Circle()
                .fill(SelectedColour)
                .frame(width: 50, height: 50)
                .position(x: I12Origin.x + 140, y: I12Origin.y - 330)
                .shadow(color: Color.black.opacity(0.2), radius: 6, y: 8)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .gesture(dragGesture)
    }
    
    var dragGesture: some Gesture {
        DragGesture(minimumDistance: 0)
            .onChanged { val in
                if StartLocation == nil {
                    StartLocation = val.location
                }
                
                let yChange = I12Origin.y + yOffset
                let distanceX = val.location.x - I12Origin.x
                let distanceY = val.location.y - yChange
                    
                let dir = CGPoint(x: distanceX, y: distanceY)
                var distance = sqrt(distanceX * distanceX + distanceY * distanceY)
                    
                if distance < radius {
                    location = val.location
                } else {
                    let clampedX = dir.x / distance * radius
                    let clampedY = dir.y / distance * radius
                    location = CGPoint(x: I12Origin.x + clampedX, y: yChange + clampedY)
                    distance = radius
                }
                    
                if distance == 0 { return }
                
                var angle = Angle(radians: -Double(atan(dir.y / dir.x)))
                if dir.x < 0 {
                    angle.degrees += 180
                }
                else if dir.x > 0 && dir.y > 0 {
                    angle.degrees += 360
                }
                
                let hue = angle.degrees / 360
                let saturation = Double(distance / radius)
                SelectedColour = Color(hue: hue, saturation: saturation, brightness: 0.9)
                let DataToSend = String(SelectedColour.description)
                self.bleManager.writeOutgoingValue(data: DataToSend)
                //bleManager.SendData = true
            }.onEnded { val in
                StartLocation = nil
                location = nil
            }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
