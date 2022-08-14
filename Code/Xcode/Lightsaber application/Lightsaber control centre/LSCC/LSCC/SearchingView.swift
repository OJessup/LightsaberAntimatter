//
//  SearchingView.swift
//  LSCC
//
//  Created by Oliver Jessup on 4/8/2022.
//

import SwiftUI

struct SearchingView: View {
    @EnvironmentObject var appState: AppState
    
    let rotationTime: Double = 0.75
    let fullRotation: Angle = .degrees(360)
    let animationTime: Double = 1.9
    static let initalDegree: Angle = .degrees(270)
    
    @State var spinnerStart: CGFloat = 0.0
    @State var spinnerEndS1: CGFloat = 0.03
    @State var spinnerEndS2S3: CGFloat = 0.03
    @State var rotationDegreeS1 = initalDegree
    @State var rotationDegreeS2 = initalDegree
    @State var rotationDegreeS3 = initalDegree
    
    @State var S1Color = Color(hue: 0.0, saturation: 1, brightness: 1)
    @State var S2Color = Color(hue: 0.136, saturation: 1, brightness: 1)
    @State var S3Color = Color(hue: 0.784, saturation: 1, brightness: 1)
    
    @State var hueS1: CGFloat = 0.0
    @State var hueS2: CGFloat = 0.137
    @State var hueS3: CGFloat = 0.784
    
    @ObservedObject var bleManager = BLEManager()
    
    var body: some View {
        ZStack {
            if bleManager.loading {
                withAnimation{
                    Text("Searching...")
                        .opacity(0.7)
                        .font(.system(size: 20, design: .rounded).weight(.light))
                        .shadow(color: Color.black.opacity(0.2), radius: 6, y: 8)
                }
            } else {
                withAnimation{
                    Text("Connected")
                        .opacity(0.7)
                        .font(.system(size: 20, design: .rounded).weight(.light))
                        .shadow(color: Color.black.opacity(0.2), radius: 6, y: 8)
                        .onAppear() {
                            DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                                appState.settingsView = false
                            }
                        }
                }
            }
            
            if bleManager.isSwitchedOn {
                Text("Make sure lightsaber is turned on and nearby")
                    .opacity(0.7)
                    .multilineTextAlignment(.center)
                    .font(.system(size: 20, design: .rounded).weight(.light))
                    .shadow(color: Color.black.opacity(0.1), radius: 6, y: 8)
                    .offset(y: 330)
                    .padding()
            } else {
                Text("Bluetooth is not turned on")
                    .offset(y: 330)
                    .foregroundColor(Color.red)
                    .font(.system(size: 20, design: .rounded).weight(.light))
                    .shadow(color: Color.black.opacity(0.1), radius: 6, y: 8)
            
            
            SpinnerCircle(start: spinnerStart, end: spinnerEndS2S3, rotation: rotationDegreeS3, color: S3Color)
            
            SpinnerCircle(start: spinnerStart, end: spinnerEndS2S3, rotation: rotationDegreeS2, color: S2Color)
            
            SpinnerCircle(start: spinnerStart, end: spinnerEndS1, rotation: rotationDegreeS1, color: S1Color)
        }
        .shadow(color: Color.black.opacity(0.1), radius: 6, y: 8)
        .frame(width: 300, height: 300)
        .onAppear() {
            Timer.scheduledTimer(withTimeInterval: animationTime, repeats: true) { (mainTimer) in
                self.animateSpinner()
            }
        }
    }
    
    func animateSpinner(with timeInterval: Double, completion: @escaping (() -> Void)) {
        Timer.scheduledTimer(withTimeInterval: timeInterval, repeats: false) { _ in
            withAnimation(Animation.easeInOut(duration: rotationTime)) {
                completion()
            }
        }
    }
    
    func animateSpinner() {
        if bleManager.loading {
            animateSpinner(with: rotationTime) { self.spinnerEndS1 = 1.0 }
            
            animateSpinner(with: (rotationTime * 2) - 0.025) {
                self.rotationDegreeS1 += fullRotation
                changeHue()
            }
            
            animateSpinner(with: (rotationTime * 2)) {
                self.spinnerEndS1 = 0.03
                changeHue()
            }
            
            animateSpinner(with: (rotationTime * 2) + 0.0525) { self.rotationDegreeS2 += fullRotation
                changeHue()}

            animateSpinner(with: (rotationTime * 2) + 0.225) { self.rotationDegreeS3 += fullRotation
                changeHue()
            }
        }
    }
    
    func changeHue() {
        let hueSpeed: CGFloat = CGFloat.random(in:0.01..<0.03)
        
        hueS1 = hueS1 + hueSpeed
        hueS2 = hueS2 + hueSpeed
        hueS3 = hueS3 + hueSpeed
        
        if hueS1 > 1.0 { hueS1 = 0.0 }
        if hueS2 > 1.0 { hueS2 = 0.0 }
        if hueS3 > 1.0 { hueS3 = 0.0 }
        
        S1Color = Color(hue: hueS1, saturation: 1, brightness: 1)
        S2Color = Color(hue: hueS2, saturation: 1, brightness: 1)
        S3Color = Color(hue: hueS3, saturation: 1, brightness: 1)
    }
}

struct SpinnerCircle: View {
    var start: CGFloat
    var end: CGFloat
    var rotation: Angle
    var color: Color
    
    var body: some View {
        Circle()
            .trim(from: start, to: end)
            .stroke(style: StrokeStyle(lineWidth: 20, lineCap: .round))
            .fill(color)
            .rotationEffect(rotation)
    }
}

struct SearchingView_Previews: PreviewProvider {
    static var previews: some View {
        SearchingView()
    }
}
