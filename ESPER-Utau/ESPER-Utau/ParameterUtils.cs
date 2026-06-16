using MathNet.Numerics.LinearAlgebra;
using System.Collections.Generic;

namespace ESPER_Utau
{
    public static class ParameterUtils
    {
        /// <summary>
        /// Creates a parameter array of the specified length with the given default value,
        /// or uses the value from argParser if the parameter is specified.
        /// </summary>
        /// <param name="args">Argument parser containing parameter flags</param>
        /// <param name="paramName">Name of the parameter to look for</param>
        /// <param name="defaultValue">Default value to use if parameter is not specified</param>
        /// <param name="arrLength">Length of the array to create</param>
        /// <returns>Vector containing the parameter values</returns>
        public static Vector<float> MakeParamArray(ArgParser args, string paramName, float defaultValue, long arrLength)
        {
            if (!args.Flags.TryGetValue(paramName, out var value))
            {
                return Vector<float>.Build.DenseOfEnumerable(Enumerable.Repeat(defaultValue, (int)arrLength));
            }
            var val = value / 100.0f;
            return Vector<float>.Build.DenseOfEnumerable(Enumerable.Repeat(val, (int)arrLength));
        }
    }
}