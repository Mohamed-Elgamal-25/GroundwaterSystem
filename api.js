/**
 * Water Quality Monitoring System - Server
 * 
 * This Express.js server handles incoming sensor data and stores it in MongoDB.
 * It maintains two types of collections:
 * 1. A main collection ('sensordatas') with the latest readings from each location
 * 2. Individual collections for each location ('location0', 'location1', 'location2') for historical data
 * 
 * The server provides a single POST endpoint (/api/mongodb) to receive and store sensor data.
 */

// Import required modules
const express = require("express");
const mongoose = require("mongoose");
const bodyParser = require("body-parser");

// Initialize Express application
const app = express();

// Middleware to parse JSON request bodies
app.use(bodyParser.json());

/**
 * MongoDB Connection Setup
 * 
 * Connects to MongoDB Atlas using the provided connection string.
 * The connection options ensure compatibility with newer MongoDB features.
 */
mongoose.connect(
    "mongodb+srv://60304859:12class34@watermonitoringsystem.quces.mongodb.net/watermonitoring", 
    { 
        useNewUrlParser: true,     // Use new URL parser
        useUnifiedTopology: true   // Use new server discovery and monitoring engine
    }
)
.then(() => console.log("Successfully connected to MongoDB"))
.catch(err => console.error("MongoDB connection error:", err));

/**
 * MongoDB Schema Definition
 * 
 * Defines the structure of documents stored in both the main and location-specific collections.
 * All sensor readings are stored as numbers, with timestamps automatically recorded.
 */
const DataSchema = new mongoose.Schema({
    location: Number,      // Location identifier (0, 1, or 2)
    temperature: Number,   // Temperature in °C
    pH: Number,            // pH level (0-14)
    TDS: Number,           // Total Dissolved Solids in ppm
    turbidity: Number,     // Turbidity in NTU
    ORP: Number,           // Oxidation-Reduction Potential in mV
    waterLevel: Number,    // Water level measurement
    timestamp: { 
        type: Date, 
        default: Date.now  // Automatically set to current time if not provided
    }
});

/**
 * MongoDB Models
 * 
 * Creates models for each collection:
 * - location0, location1, location2: Individual collections for each monitoring location
 * - sensordatas: Main collection that keeps only the latest reading from each location
 */
const Location0 = mongoose.model("location0", DataSchema);  // Collection for location 0 (Doha)
const Location1 = mongoose.model("location1", DataSchema);  // Collection for location 1 (Al Khor)
const Location2 = mongoose.model("location2", DataSchema);  // Collection for location 2 (Al Wakrah)
const Data = mongoose.model("sensordatas", DataSchema);     // Main collection for latest readings

/**
 * POST /api/mongodb
 * 
 * Endpoint to receive sensor data from monitoring devices.
 * Stores data in both the main collection (latest readings) and the appropriate location-specific collection (historical data).
 * 
 * Request Body Parameters:
 * - location: Integer (0, 1, or 2) - The monitoring location ID
 * - temperature: Number - Temperature reading
 * - pH: Number - pH level reading
 * - TDS: Number - Total Dissolved Solids reading
 * - turbidity: Number - Turbidity reading
 * - ORP: Number - Oxidation-Reduction Potential reading
 * - waterLevel: Number - Water level reading
 * - timestamp: String (ISO format) - Time of reading (optional, defaults to current time)
 * 
 * Response:
 * - Success: 200 OK with success message and updated document
 * - Error: 500 Internal Server Error with error message
 */
app.post("/api/mongodb", async (req, res) => {
    try {
        // Destructure required fields from request body
        const { location, temperature, pH, TDS, turbidity, waterLevel, ORP, timestamp } = req.body;

        // Validate required fields
        if (location === undefined || location === null) {
            throw new Error("Location is required");
        }

        /**
         * Update Main Collection (sensordatas)
         * 
         * Uses findOneAndUpdate with upsert to:
         * - Update existing document if location exists
         * - Create new document if location doesn't exist
         */
        const updatedData = await Data.findOneAndUpdate(
            { location: location },  // Filter by location
            { 
                temperature, 
                pH, 
                TDS, 
                turbidity, 
                ORP, 
                waterLevel,
                timestamp: timestamp ? new Date(timestamp) : new Date()
            },
            { 
                upsert: true,    // Create document if it doesn't exist
                new: true        // Return the updated document
            }
        );

        /**
         * Store in Location-Specific Collection
         * 
         * Selects the appropriate model based on location number
         * and creates a new document with the sensor readings.
         */
        let locationModel;
        switch(location) {
            case 0:
                locationModel = Location0;
                break;
            case 1:
                locationModel = Location1;
                break;
            case 2:
                locationModel = Location2;
                break;
            default:
                throw new Error("Invalid location number. Must be 0, 1, or 2");
        }

        // Create new document in location-specific collection
        await locationModel.create({
            location,
            temperature,
            pH,
            TDS,
            turbidity,
            ORP,
            waterLevel,
            timestamp: timestamp ? new Date(timestamp) : new Date()
        });

        // Send success response
        res.json({ 
            message: "Data updated successfully in both collections",
            data: updatedData
        });

    } catch (error) {
        // Handle errors and send appropriate response
        console.error("Error processing sensor data:", error);
        res.status(500).json({ 
            error: "Failed to process sensor data",
            details: error.message 
        });
    }
});

/**
 * Start Server
 * 
 * Listens on port 3000 for incoming requests.
 */
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
    console.log(`API endpoint available at http://localhost:${PORT}/api/mongodb`);
});

// Export the app for testing purposes
module.exports = app;