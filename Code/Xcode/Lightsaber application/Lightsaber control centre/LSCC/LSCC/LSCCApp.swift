//
//  LSCCApp.swift
//  LSCC
//
//  Created by Oliver Jessup on 3/8/2022.
//

import SwiftUI

class AppState: ObservableObject {
    @Published var settingsView: Bool
    
    init(settingsView: Bool) {
        self.settingsView = settingsView
    }
}

@main
struct LSCCApp: App {
    @ObservedObject var appState = AppState(settingsView: true)
    var body: some Scene {
        WindowGroup {
            if appState.settingsView {
                SearchingView()
                    .environmentObject(appState)
            } else {
                ContentView()
                    .environmentObject(appState)
            }
        }
    }
}
