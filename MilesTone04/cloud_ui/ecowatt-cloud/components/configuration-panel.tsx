"use client"

import React, { useState } from "react"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Switch } from "@/components/ui/switch"
import { AlertCircle, CheckCircle2 } from "lucide-react"
import { API_ENDPOINTS } from "@/lib/config"

export default function ConfigurationPanel() {
  const [regRead, setRegRead] = useState([1, 1, 1, 1, 1, 1, 1, 1, 1, 1])
  const [interval, setInterval] = useState(1000)
  const [version, setVersion] = useState("1.0.0")
  const [loading, setLoading] = useState(false)
  const [message, setMessage] = useState<{ type: "success" | "error"; text: string } | null>(null)
  // Command queue state
  const [commands, setCommands] = useState<Array<{ action: string; target_register: number; value: number }>>([])
  const [cmdReg, setCmdReg] = useState<string>("")
  const [cmdVal, setCmdVal] = useState<string>("")
  const [cmdMessage, setCmdMessage] = useState<{ type: "success" | "error"; text: string } | null>(null)

  const fetchCommands = async () => {
    setCmdMessage(null)
    try {
      const res = await fetch(API_ENDPOINTS.commands)
      const data = await res.json()
      if (data && Array.isArray(data.commands)) {
        setCommands(data.commands)
        setCmdMessage({ type: "success", text: `Fetched ${data.commands.length} commands` })
      } else {
        setCommands([])
        setCmdMessage({ type: "error", text: "Invalid commands response" })
      }
    } catch (e) {
      setCmdMessage({ type: "error", text: "Error fetching commands" })
    }
  }

  const addCommand = async () => {
    setCmdMessage(null)
    const reg = Number.parseInt(cmdReg)
    const val = Number.parseInt(cmdVal)
    if (!Number.isInteger(reg) || reg < 0 || reg > 9) {
      setCmdMessage({ type: "error", text: "Register must be an integer between 0 and 9" })
      return
    }
    if (!Number.isFinite(val)) {
      setCmdMessage({ type: "error", text: "Value must be a valid number" })
      return
    }
    try {
      const res = await fetch(API_ENDPOINTS.commands, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "write_register", target_register: reg, value: val }),
      })
      const data = await res.json()
      if (data.status === "success") {
        setCmdMessage({ type: "success", text: "Command queued" })
        setCmdReg("")
        setCmdVal("")
        // Optionally refresh queue
        fetchCommands()
      } else {
        setCmdMessage({ type: "error", text: data.message || "Failed to queue command" })
      }
    } catch (e) {
      setCmdMessage({ type: "error", text: "Error queuing command" })
    }
  }

  // Fetch current configuration on mount
  React.useEffect(() => {
    const fetchConfig = async () => {
      try {
        const response = await fetch(API_ENDPOINTS.config)
        const data = await response.json()
        if (data.status === "success" && data.config) {
          setRegRead(data.config.reg_read)
          setInterval(data.config.interval)
          setVersion(data.config.version)
        } else {
          setMessage({ type: "error", text: "Failed to fetch configuration" })
        }
      } catch (error) {
        setMessage({ type: "error", text: "Error fetching configuration: " + error })
      }
    }
    fetchConfig()
  }, [])

  const handleToggleRegister = (index: number) => {
    const newRegRead = [...regRead]
    newRegRead[index] = newRegRead[index] === 1 ? 0 : 1
    setRegRead(newRegRead)
  }

  const handleSave = async () => {
    setLoading(true)
    setMessage(null)
    try {
      const response = await fetch(API_ENDPOINTS.config, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ reg_read: regRead, interval }),
      })
      const data = await response.json()
      if (data.status === "success") {
        setMessage({ type: "success", text: "Configuration updated successfully" })
        // Do not update version here
      } else {
        setMessage({ type: "error", text: "Failed to update configuration" })
      }
    } catch (error) {
      setMessage({ type: "error", text: "An error occurred while saving configuration.  Error is " + error })
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="space-y-6">
      <Card>
        <CardHeader>
          <CardTitle>Device Configuration</CardTitle>
          <CardDescription>Manage register settings and polling interval</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          {/* Version Info */}
          <div className="rounded-lg bg-muted/50 p-4">
            <p className="text-sm text-muted-foreground">Current Version</p>
            <p className="text-lg font-semibold text-foreground">{version}</p>
          </div>

          {/* Register Toggles */}
          <div className="space-y-4">
            <Label className="text-base font-semibold">Register Configuration</Label>
            <div className="grid grid-cols-2 gap-4 sm:grid-cols-5">
              {regRead.map((value, index) => (
                <div
                  key={index}
                  className="flex items-center justify-between rounded-lg border border-border bg-card p-3"
                >
                  <span className="text-sm font-medium">Reg {index}</span>
                  <Switch checked={value === 1} onCheckedChange={() => handleToggleRegister(index)} />
                </div>
              ))}
            </div>
          </div>

          {/* Interval Setting */}
          <div className="space-y-2">
            <Label htmlFor="interval">Polling Interval (ms)</Label>
            <Input
              id="interval"
              type="number"
              value={interval}
              onChange={(e) => setInterval(Number.parseInt(e.target.value) || 1000)}
              min={100}
              step={100}
              className="max-w-xs"
            />
            <p className="text-xs text-muted-foreground">Minimum 100ms, recommended 1000ms</p>
          </div>

          {/* Status Message */}
          {message && (
            <div
              className={`flex items-center gap-2 rounded-lg p-3 ${
                message.type === "success"
                  ? "bg-green-50 text-green-900 dark:bg-green-950 dark:text-green-200"
                  : "bg-red-50 text-red-900 dark:bg-red-950 dark:text-red-200"
              }`}
            >
              {message.type === "success" ? <CheckCircle2 className="h-4 w-4" /> : <AlertCircle className="h-4 w-4" />}
              <span className="text-sm">{message.text}</span>
            </div>
          )}

          {/* Save Button */}
          <Button
            onClick={handleSave}
            disabled={loading}
            className="w-full bg-primary hover:bg-primary/90 text-primary-foreground"
          >
            {loading ? "Saving..." : "Save Configuration"}
          </Button>
        </CardContent>
      </Card>
      {/* Command Queue */}
      <Card>
        <CardHeader>
          <CardTitle>Command Queue</CardTitle>
          <CardDescription>View and queue commands for the device</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          {/* Fetch queue */}
          <div className="flex gap-2">
            <Button onClick={fetchCommands} variant="outline">Fetch Command Queue</Button>
          </div>
          {/* Status */}
          {cmdMessage && (
            <div
              className={`flex items-center gap-2 rounded-lg p-3 ${
                cmdMessage.type === "success"
                  ? "bg-green-50 text-green-900 dark:bg-green-950 dark:text-green-200"
                  : "bg-red-50 text-red-900 dark:bg-red-950 dark:text-red-200"
              }`}
            >
              {cmdMessage.type === "success" ? <CheckCircle2 className="h-4 w-4" /> : <AlertCircle className="h-4 w-4" />}
              <span className="text-sm">{cmdMessage.text}</span>
            </div>
          )}
          {/* Queue list */}
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-border">
                  <th className="px-4 py-2 text-left font-semibold text-foreground">Action</th>
                  <th className="px-4 py-2 text-left font-semibold text-foreground">Target Register</th>
                  <th className="px-4 py-2 text-left font-semibold text-foreground">Value</th>
                </tr>
              </thead>
              <tbody>
                {commands.length > 0 ? (
                  commands.map((c, idx) => (
                    <tr key={idx} className="border-b border-border hover:bg-muted/50">
                      <td className="px-4 py-2 text-foreground">{c.action}</td>
                      <td className="px-4 py-2 text-foreground">{c.target_register}</td>
                      <td className="px-4 py-2 font-mono text-primary">{c.value}</td>
                    </tr>
                  ))
                ) : (
                  <tr>
                    <td colSpan={3} className="px-4 py-6 text-center text-muted-foreground">No commands queued</td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>

          {/* Add command */}
          <div className="space-y-3">
            <Label className="text-base font-semibold">Add Register Write</Label>
            <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 items-end">
              <div className="space-y-2">
                <Label htmlFor="cmd-register">Register (0-9)</Label>
                <Input id="cmd-register" type="number" value={cmdReg} onChange={(e) => setCmdReg(e.target.value)} min={0} max={9} />
              </div>
              <div className="space-y-2">
                <Label htmlFor="cmd-value">Value</Label>
                <Input id="cmd-value" type="number" value={cmdVal} onChange={(e) => setCmdVal(e.target.value)} />
              </div>
              <Button onClick={addCommand} className="bg-primary hover:bg-primary/90 text-primary-foreground">Add</Button>
            </div>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
