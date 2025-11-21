import mongoose from "mongoose";

const TestCaseSchema = new mongoose.Schema({
    input: String,
    expected: String
});

const ProblemSchema = new mongoose.Schema({
    title: String,
    category: String,
    description: String,
    cases: [TestCaseSchema]
});

export default mongoose.model("Problem", ProblemSchema);
