//
//  ContentView.swift
//  LSCC v1.0
//
//  Created by Oliver Jessup on 8/8/2022.
//

import SwiftUI

struct ContentView: View {
    
    var bluetooth = BLEcontroller.shared
    
    @State var response = Data()
    @State var string: String = ""
    @State var value: Float = 0
    
    var body: some View {
        Text("Hello")
    }
}
