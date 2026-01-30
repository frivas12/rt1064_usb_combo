using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MCM6000_UI.builders
{
    public interface IOneWireCompiler
    {
        public Task<bool> CompileToFileAsync(string outputFilePath, object compilationArguments,
            CancellationToken token);

        public Task<MemoryStream> CompileToMemoryAsync(object compilationArguments,
            CancellationToken token);
    }

    public interface IOneWireCompiler<TCompilerArgs>
        where TCompilerArgs : class
    {
        public Task<bool> CompileToFileAsync(
            string outputFilePath, TCompilerArgs compilationArguments,
            CancellationToken token);

        public Task<MemoryStream> CompileToMemoryAsync(
            TCompilerArgs compilationArguments,
            CancellationToken token);
    }


    /// <summary>
    /// Adapts a typed one-wire compiler down to a type-erased compiler.
    /// </summary>
    public sealed class TypeErasedOneWireCompilerAdapter<TCompilerArgs> : IOneWireCompiler
        where TCompilerArgs : class
    {
        public Task<bool> CompileToFileAsync(string outputFilePath, object compilationArguments, CancellationToken token) =>
            _compiler.CompileToFileAsync(outputFilePath, (TCompilerArgs)compilationArguments, token);

        public Task<MemoryStream> CompileToMemoryAsync(object compilationArguments, CancellationToken token) =>
            _compiler.CompileToMemoryAsync((TCompilerArgs)compilationArguments, token);


        public TypeErasedOneWireCompilerAdapter(IOneWireCompiler<TCompilerArgs> typedCompiler)
        {
            _compiler = typedCompiler;
        }





        private readonly IOneWireCompiler<TCompilerArgs> _compiler;
    }
}
