// Copyright 2010-2021 Google LLC
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// [START program]
// OR-Tools solution to the N-queens problem.
// [START import]
using System;
using Google.OrTools.Sat;
// [END import]

public class NQueensSat
{
    // [START solution_printer]
    public class SolutionPrinter : CpSolverSolutionCallback
    {
        public SolutionPrinter(IntVar[] queens)
        {
            queens_ = queens;
        }

        public override void OnSolutionCallback()
        {
            Console.WriteLine($"Solution {SolutionCount_}");
            for (int i = 0; i < queens_.Length; ++i)
            {
                for (int j = 0; j < queens_.Length; ++j)
                {
                    if (Value(queens_[j]) == i)
                    {
                        Console.Write("Q");
                    }
                    else
                    {
                        Console.Write("_");
                    }
                    if (j != queens_.Length - 1)
                        Console.Write(" ");
                }
                Console.WriteLine("");
            }
            SolutionCount_++;
        }

        public int SolutionCount()
        {
            return SolutionCount_;
        }

        private int SolutionCount_;
        private IntVar[] queens_;
    }
    // [END solution_printer]

    static void Main()
    {
        // Constraint programming engine
        // [START model]
        CpModel model = new CpModel();
        // [START model]

        // [START variables]
        int BoardSize = 8;
        IntVar[] queens = new IntVar[BoardSize];
        for (int i = 0; i < BoardSize; ++i)
        {
            queens[i] = model.NewIntVar(0, BoardSize - 1, $"x{i}");
        }
        // [END variables]

        // Define constraints.
        // [START constraints]
        // All rows must be different.
        model.AddAllDifferent(queens);

        // All columns must be different because the indices of queens are all different.
        // No two queens can be on the same diagonal.
        IntVar[] diag1 = new IntVar[BoardSize];
        IntVar[] diag2 = new IntVar[BoardSize];
        for (int i = 0; i < BoardSize; ++i)
        {
          IntVar tmp1 = model.NewIntVar(0, BoardSize * 2, $"x{i}");
          model.Add(LinearExpr.Sum(new IntVar[]{queens[i], model.NewConstant(i)}) == tmp1);
          diag1[i] = tmp1;

          IntVar tmp2 = model.NewIntVar(-BoardSize, BoardSize, $"x{i}");
          model.Add(LinearExpr.Sum(new IntVar[]{queens[i], model.NewConstant(-i)}) == tmp2);
          diag2[i] = tmp2;
        }

        model.AddAllDifferent(diag1);
        model.AddAllDifferent(diag2);
        // [END constraints]

        // [START solve]
        // Creates a solver and solves the model.
        CpSolver solver = new CpSolver();
        SolutionPrinter cb = new SolutionPrinter(queens);
        // Search for all solutions.
        solver.StringParameters = "enumerate_all_solutions:true";
        // And solve.
        solver.Solve(model, cb);
        // [END solve]

        // [START statistics]
        Console.WriteLine("Statistics");
        Console.WriteLine($"  conflicts : {solver.NumConflicts()}");
        Console.WriteLine($"  branches  : {solver.NumBranches()}");
        Console.WriteLine($"  wall time : {solver.WallTime()} s");
        Console.WriteLine($"  number of solutions found: {cb.SolutionCount()}");
        // [END statistics]
    }
}
// [END program]
