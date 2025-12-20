"use client"

import { useState } from "react"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import ConfigurationPanel from "@/components/configuration-panel"
import DataMonitoring from "@/components/data-monitoring"
import FirmwareManagement from "@/components/firmware-management"

export default function Home() {
  const [activeTab, setActiveTab] = useState("data")

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="border-b border-border bg-card">
        <div className="mx-auto max-w-7xl px-6 py-6">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-bold text-foreground">EcoWatt Cloud</h1>
              <p className="mt-1 text-sm text-muted-foreground">Leptons Device Management</p>
            </div>
            <div className="flex items-center gap-2">
              <div className="h-3 w-3 rounded-full bg-primary animate-pulse"></div>
              <span className="text-sm text-muted-foreground">Connected</span>
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="mx-auto max-w-7xl px-6 py-8">
        <Tabs value={activeTab} onValueChange={setActiveTab} className="w-full">
          <TabsList className="grid w-full grid-cols-3 mb-8">
            <TabsTrigger value="config">Configuration</TabsTrigger>
            <TabsTrigger value="data">Data Monitoring</TabsTrigger>
            <TabsTrigger value="firmware">Firmware</TabsTrigger>
          </TabsList>

          <TabsContent value="config" className="space-y-6">
            <ConfigurationPanel />
          </TabsContent>

          <TabsContent value="data" className="space-y-6">
            <DataMonitoring />
          </TabsContent>

          <TabsContent value="firmware" className="space-y-6">
            <FirmwareManagement />
          </TabsContent>
        </Tabs>
      </main>
    </div>
  )
}
