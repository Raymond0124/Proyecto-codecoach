import express from "express";
import mongoose from "mongoose";
import cors from "cors";
import problemRoutes from "./routes/problems.js";

const app = express();
app.use(cors());
app.use(express.json());

mongoose.connect("mongodb://localhost:27017/codecoach")
    .then(() => console.log("MongoDB conectado"))
    .catch(err => console.error("❌ Error conectando MongoDB:", err));

app.use("/problems", problemRoutes);

app.listen(3000, () => {
    console.log("🚀 Servidor corriendo en http://localhost:3000");
});
