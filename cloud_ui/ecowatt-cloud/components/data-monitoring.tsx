"use client"

import React, { useState } from "react"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { AlertCircle, CheckCircle2, ChevronLeft, ChevronRight } from "lucide-react"
import { API_ENDPOINTS } from "@/lib/config"

export default function DataMonitoring() {
  // Load data on mount
  React.useEffect(() => {
    fetchLatestData(1)
  }, [])
  const [latestRegisters, setLatestRegisters] = useState<any[]>([])
  const [latestRecords, setLatestRecords] = useState<any[]>([])
  const [totalCount, setTotalCount] = useState(0)
  const [currentPage, setCurrentPage] = useState(1)
  const [loading, setLoading] = useState(false)
  const [message, setMessage] = useState<{ type: "success" | "error"; text: string } | null>(null)
  const [queryMode, setQueryMode] = useState<"latest" | "range">("latest")
  const [startId, setStartId] = useState("")
  const [endId, setEndId] = useState("")
  const [startTime, setStartTime] = useState("")
  const [endTime, setEndTime] = useState("")

  const pageSize = 20
  const totalPages = Math.ceil(totalCount / pageSize)

  // Fetch latest register values and latest records by record id batches
  const fetchLatestData = async (batch = 1) => {
    setLoading(true)
    setMessage(null)
    try {
      // Get latest values for each register
      const regResponse = await fetch(API_ENDPOINTS.data)
      const regData = await regResponse.json()
      let registers = Array(10).fill(null)
      if (regData.status === "success" && Array.isArray(regData.data)) {
        // Fill registers by register index, fallback to null if not found
        regData.data.forEach((item: any) => {
          if (typeof item.register === 'number' && item.register >= 0 && item.register < 10) {
            registers[item.register] = item
          }
        })
      }
      setLatestRegisters(registers)

      // Get total count
      const countResponse = await fetch(API_ENDPOINTS.dataCount)
      const countData = await countResponse.json()
      const count = countData.count || 0
      setTotalCount(count)

      // Calculate newest record id (assuming IDs are sequential and start at 1)
      const newestId = count
      const startId = Math.max(1, newestId - (batch - 1) * pageSize - pageSize + 1)
      const endId = newestId - (batch - 1) * pageSize
      // Fetch records in this batch (descending order)
      let recUrl = `${API_ENDPOINTS.data}?start_id=${startId}&end_id=${endId}`
      const recResponse = await fetch(recUrl)
      const recData = await recResponse.json()
      let records = []
      if (recData.status === "success" && Array.isArray(recData.data)) {
        // Sort descending by id
        records = recData.data.sort((a: any, b: any) => b.id - a.id)
      }
      setLatestRecords(records)
      setCurrentPage(batch)
      setMessage({ type: "success", text: `Showing records ${startId} to ${endId} of ${count}` })
    } catch (error) {
      setMessage({ type: "error", text: "Error fetching data" })
    } finally {
      setLoading(false)
    }
  }

  const fetchDataByRange = async (page = 1) => {
    setLoading(true)
    setMessage(null)
    try {
      const params = new URLSearchParams()
      if (startId) params.append("start_id", startId)
      if (endId) params.append("end_id", endId)
      if (startTime) params.append("start_time", startTime)
      if (endTime) params.append("end_time", endTime)

      const countParams = new URLSearchParams(params)
      const countResponse = await fetch(`${API_ENDPOINTS.dataCount}?${countParams}`)
      const countData = await countResponse.json()
      const count = countData.count || 0
      setTotalCount(count)

      // Fetch paginated data
      const offset = (page - 1) * pageSize
      params.append("limit", pageSize.toString())
      params.append("offset", offset.toString())

      const response = await fetch(`${API_ENDPOINTS.data}?${params}`)
      const data = await response.json()
      if (data.status === "success") {
  // setLatestData removed, now handled by setLatestRegisters and setLatestRecords
        setCurrentPage(page)
        setMessage({ type: "success", text: `Retrieved ${data.data?.length || 0} of ${count} records` })
      } else {
        setMessage({ type: "error", text: "Failed to fetch data" })
      }
    } catch (error) {
      setMessage({ type: "error", text: "Error fetching data" })
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="space-y-6">
      <Card>
        <CardHeader>
          <CardTitle>Data Monitoring</CardTitle>
          <CardDescription>View and query sensor data from registers</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          {/* Query Tabs */}
          <Tabs value={queryMode} onValueChange={(v) => setQueryMode(v as "latest" | "range")}>
            <TabsList className="grid w-full grid-cols-2">
              <TabsTrigger value="latest">Latest Values</TabsTrigger>
              <TabsTrigger value="range">Query Range</TabsTrigger>
            </TabsList>

            <TabsContent value="latest" className="space-y-4 mt-4">
              <Button
                onClick={() => fetchLatestData(1)}
                disabled={loading}
                className="w-full bg-primary hover:bg-primary/90 text-primary-foreground"
              >
                {loading ? "Loading..." : "Fetch Latest Data"}
              </Button>
            </TabsContent>

            <TabsContent value="range" className="space-y-4 mt-4">
              <div className="grid grid-cols-2 gap-4">
                <div className="space-y-2">
                  <Label htmlFor="start-id">Start ID</Label>
                  <Input
                    id="start-id"
                    type="number"
                    value={startId}
                    onChange={(e) => setStartId(e.target.value)}
                    placeholder="e.g., 100"
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="end-id">End ID</Label>
                  <Input
                    id="end-id"
                    type="number"
                    value={endId}
                    onChange={(e) => setEndId(e.target.value)}
                    placeholder="e.g., 200"
                  />
                </div>
              </div>
              <div className="grid grid-cols-2 gap-4">
                <div className="space-y-2">
                  <Label htmlFor="start-time">Start Time</Label>
                  <Input
                    id="start-time"
                    type="datetime-local"
                    value={startTime}
                    onChange={(e) => setStartTime(e.target.value)}
                  />
                </div>
                <div className="space-y-2">
                  <Label htmlFor="end-time">End Time</Label>
                  <Input
                    id="end-time"
                    type="datetime-local"
                    value={endTime}
                    onChange={(e) => setEndTime(e.target.value)}
                  />
                </div>
              </div>
              <Button
                onClick={() => fetchDataByRange(1)}
                disabled={loading}
                className="w-full bg-primary hover:bg-primary/90 text-primary-foreground"
              >
                {loading ? "Loading..." : "Query Data"}
              </Button>
            </TabsContent>
          </Tabs>

          {totalCount > 0 && (
            <div className="flex items-center justify-between gap-4">
              <div className="text-sm font-semibold text-foreground">
                Page {currentPage} of {totalPages} ({totalCount} total records)
              </div>
              <div className="flex gap-2">
                <Button
                  onClick={() =>
                    queryMode === "latest" ? fetchLatestData(currentPage - 1) : fetchDataByRange(currentPage - 1)
                  }
                  disabled={loading || currentPage === 1}
                  variant="outline"
                  size="sm"
                  className="gap-1"
                >
                  <ChevronLeft className="h-4 w-4" />
                  Previous
                </Button>

                <div className="flex gap-1">
                  {Array.from({ length: Math.min(totalPages, 5) }, (_, i) => {
                    const pageNum = i + 1
                    return (
                      <Button
                        key={pageNum}
                        onClick={() => (queryMode === "latest" ? fetchLatestData(pageNum) : fetchDataByRange(pageNum))}
                        disabled={loading}
                        variant={currentPage === pageNum ? "default" : "outline"}
                        size="sm"
                        className="w-8 h-8 p-0"
                      >
                        {pageNum}
                      </Button>
                    )
                  })}
                  {totalPages > 5 && <span className="flex items-center px-2 text-muted-foreground">...</span>}
                </div>

                <Button
                  onClick={() =>
                    queryMode === "latest" ? fetchLatestData(currentPage + 1) : fetchDataByRange(currentPage + 1)
                  }
                  disabled={loading || currentPage === totalPages}
                  variant="outline"
                  size="sm"
                  className="gap-1"
                >
                  Next
                  <ChevronRight className="h-4 w-4" />
                </Button>
              </div>
            </div>
          )}

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

          {/* Latest Register Values Table */}
          <div className="overflow-x-auto mb-8">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-border">
                  <th className="px-4 py-2 text-left font-semibold text-foreground">Register</th>
                  <th className="px-4 py-2 text-left font-semibold text-foreground">Value</th>
                  <th className="px-4 py-2 text-left font-semibold text-foreground">Timestamp</th>
                </tr>
              </thead>
              <tbody>
                {latestRegisters.map((item, idx) => (
                  <tr key={idx} className="border-b border-border hover:bg-muted/50">
                    <td className="px-4 py-2 text-foreground">{`Register ${idx}`}</td>
                    <td className="px-4 py-2 font-mono text-primary">{item && item.value !== undefined ? item.value : 'N/A'}</td>
                    <td className="px-4 py-2 text-muted-foreground text-xs">{item && item.timestamp ? new Date(item.timestamp).toLocaleString() : 'N/A'}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Latest 20 Records Table with Pagination */}
          {latestRecords.length > 0 ? (
            <div className="overflow-x-auto">
              <table className="w-full text-sm">
                <thead>
                  <tr className="border-b border-border">
                    <th className="px-4 py-2 text-left font-semibold text-foreground">ID</th>
                    <th className="px-4 py-2 text-left font-semibold text-foreground">Timestamp</th>
                    <th className="px-4 py-2 text-left font-semibold text-foreground">Data</th>
                  </tr>
                </thead>
                <tbody>
                  {latestRecords.map((item, idx) => (
                    <tr key={idx} className="border-b border-border hover:bg-muted/50">
                      <td className="px-4 py-2 text-foreground">{item.id}</td>
                      <td className="px-4 py-2 text-muted-foreground text-xs">{new Date(item.timestamp).toLocaleString()}</td>
                      <td className="px-4 py-2 font-mono text-primary">{Array.isArray(item.data) ? item.data.join(", ") : item.data}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
              {/* Pagination Controls */}
              <div className="flex items-center justify-between gap-4 mt-4">
                <div className="text-sm font-semibold text-foreground">
                  Batch {currentPage} ({totalCount} total records)
                </div>
                <div className="flex gap-2">
                  <Button
                    onClick={() => fetchLatestData(currentPage + 1)}
                    disabled={loading || (totalCount - currentPage * pageSize) < 1}
                    variant="outline"
                    size="sm"
                    className="gap-1"
                  >
                    <ChevronLeft className="h-4 w-4" />
                    Older
                  </Button>
                  <Button
                    onClick={() => fetchLatestData(currentPage - 1)}
                    disabled={loading || currentPage === 1}
                    variant="outline"
                    size="sm"
                    className="gap-1"
                  >
                    Newer
                    <ChevronRight className="h-4 w-4" />
                  </Button>
                </div>
              </div>
            </div>
          ) : (
            <div className="text-center text-muted-foreground">Not available</div>
          )}
        </CardContent>
      </Card>
    </div>
  )
}
