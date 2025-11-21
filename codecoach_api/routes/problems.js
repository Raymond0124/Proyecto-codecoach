import express from "express";
import Problem from "../models/Problem.js";

const router = express.Router();

// POST /problems
router.post("/", async (req, res) => {
    try {
        const newProblem = new Problem(req.body);
        await newProblem.save();
        return res.status(201).json({ message: "Problem created", id: newProblem._id });
    } catch (err) {
        console.error("Error al guardar:", err);
        return res.status(500).json({ error: "Database error", details: err });
    }
});

// GET /problems/random?category=x
router.get("/random", async (req, res) => {
    try {
        const category = req.query.category;
        const filter = category ? { category } : {};
        const count = await Problem.countDocuments(filter);
        const randomIndex = Math.floor(Math.random() * count);
        const problem = await Problem.findOne(filter).skip(randomIndex);
        res.json(problem);
    } catch (err) {
        res.status(500).json({ error: "Database error" });
    }
});

export default router;
