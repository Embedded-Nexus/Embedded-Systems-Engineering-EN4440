// Centralized API configuration
// Update BASE_URL here to change the API endpoint for all components

export const BASE_URL = "http://127.0.0.1:5000"

export const API_ENDPOINTS = {
  config: `${BASE_URL}/config`,
  firmware: `${BASE_URL}/firmware`,
  data: `${BASE_URL}/data`,
  dataCount: `${BASE_URL}/data/count`,
  commands: `${BASE_URL}/commands`,
}
