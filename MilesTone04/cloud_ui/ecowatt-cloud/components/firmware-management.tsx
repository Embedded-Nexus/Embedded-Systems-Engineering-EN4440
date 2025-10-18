"use client"

import React, { useState, useRef } from "react"

import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import { AlertCircle, CheckCircle2, Download, Upload } from "lucide-react"
import { API_ENDPOINTS } from "@/lib/config"

export default function FirmwareManagement() {
  const [currentVersion, setCurrentVersion] = useState("1.0.0")

  // Fetch current version from config on mount
  React.useEffect(() => {
    const fetchConfigVersion = async () => {
      try {
        const response = await fetch(API_ENDPOINTS.config)
        const data = await response.json()
        if (data.status === "success" && data.config && data.config.version) {
          setCurrentVersion(data.config.version)
        }
      } catch (e) {
        // ignore error, fallback to default version
      }
    }
    fetchConfigVersion()
  }, [])
  const [updateLevel, setUpdateLevel] = useState("1")
  const [loading, setLoading] = useState(false)
  const [message, setMessage] = useState<{ type: "success" | "error"; text: string } | null>(null)
  const fileInputRef = useRef<HTMLInputElement>(null)

  const handleFileUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (!file) return

    setLoading(true)
    setMessage(null)

    try {
      const formData = new FormData()
      formData.append("file", file)
      formData.append("level", updateLevel)

      const response = await fetch(API_ENDPOINTS.firmware, {
        method: "POST",
        body: formData,
      })

      const data = await response.json()
      if (data.status === "success") {
        setMessage({ type: "success", text: `Firmware uploaded successfully (v${data.version})` })
        setCurrentVersion(data.version)
      } else {
        setMessage({ type: "error", text: "Failed to upload firmware" })
      }
    } catch (error) {
      setMessage({ type: "error", text: "Error uploading firmware" })
    } finally {
      setLoading(false)
      if (fileInputRef.current) fileInputRef.current.value = ""
    }
  }

  const handleDownload = async () => {
    setLoading(true)
    try {
      const response = await fetch(API_ENDPOINTS.firmware)
      if (response.ok) {
        const blob = await response.blob()
        const url = window.URL.createObjectURL(blob)
        const a = document.createElement("a")
        a.href = url
        a.download = `firmware-${currentVersion}.bin`
        document.body.appendChild(a)
        a.click()
        window.URL.revokeObjectURL(url)
        document.body.removeChild(a)
        setMessage({ type: "success", text: "Firmware downloaded successfully" })
      } else {
        setMessage({ type: "error", text: "Failed to download firmware" })
      }
    } catch (error) {
      setMessage({ type: "error", text: "Error downloading firmware" })
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="space-y-6">
      <Card>
        <CardHeader>
          <CardTitle>Firmware Management</CardTitle>
          <CardDescription>Upload new firmware or download the current version</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          {/* Current Version */}
          <div className="rounded-lg bg-muted/50 p-4">
            <p className="text-sm text-muted-foreground">Current Firmware Version</p>
            <p className="text-2xl font-bold text-foreground">{currentVersion}</p>
          </div>

          {/* Download Section */}
          <div className="space-y-3">
            <Label className="text-base font-semibold">Download Firmware</Label>
            <Button onClick={handleDownload} disabled={loading} variant="outline" className="w-full bg-transparent">
              <Download className="mr-2 h-4 w-4" />
              {loading ? "Downloading..." : "Download Current Firmware"}
            </Button>
          </div>

          {/* Divider */}
          <div className="relative">
            <div className="absolute inset-0 flex items-center">
              <div className="w-full border-t border-border"></div>
            </div>
            <div className="relative flex justify-center text-xs uppercase">
              <span className="bg-card px-2 text-muted-foreground">Or</span>
            </div>
          </div>

          {/* Upload Section */}
          <div className="space-y-4">
            <Label className="text-base font-semibold">Upload New Firmware</Label>

            <div className="space-y-2">
              <Label htmlFor="update-level">Update Level</Label>
              <Select value={updateLevel} onValueChange={setUpdateLevel}>
                <SelectTrigger>
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="1">Level 1 - Minor Update</SelectItem>
                  <SelectItem value="2">Level 2 - Standard Update</SelectItem>
                  <SelectItem value="3">Level 3 - Major Update</SelectItem>
                </SelectContent>
              </Select>
            </div>

            <div className="space-y-2">
              <Label htmlFor="firmware-file">Firmware File (.bin)</Label>
              <div className="flex gap-2">
                <Input
                  ref={fileInputRef}
                  id="firmware-file"
                  type="file"
                  accept=".bin"
                  onChange={handleFileUpload}
                  disabled={loading}
                  className="flex-1"
                />
                <Button
                  onClick={() => fileInputRef.current?.click()}
                  disabled={loading}
                  className="bg-primary hover:bg-primary/90 text-primary-foreground"
                >
                  <Upload className="mr-2 h-4 w-4" />
                  Browse
                </Button>
              </div>
            </div>
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
        </CardContent>
      </Card>
    </div>
  )
}
