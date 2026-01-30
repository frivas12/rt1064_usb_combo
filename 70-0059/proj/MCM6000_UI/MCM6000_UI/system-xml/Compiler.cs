using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace SystemXMLParser
{
    public sealed class Compiler
    {
        const string DLL_PATH = "compiler.dll";

#if false    // Compiler option for debug flag
        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        public static extern int lutcompiler_get_last_error();
#endif

        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern int lutcompiler_get_output_size(string s_input_file);

        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern int lutcompiler_get_output_size_raw([In] byte[] text_input, uint text_length);

        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool lutcompiler_compile(string s_input_file, [In, Out] byte[] p_buffer);

        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool lutcompiler_compile_raw([In] byte[] text_input, uint text_length, [In, Out] byte[] p_buffer);

        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool lutcompiler_compile_to_file(string s_input_file, string s_output_file);

        [DllImport(DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool lutcompiler_compile_to_file_raw([In] byte[] text_input, uint text_length, string s_output_file);

        public static byte[]? Compile(string input_file)
        {
            int bytes_to_allocate = lutcompiler_get_output_size(input_file);
            if (bytes_to_allocate != -1)
            {
                byte[] rt = new byte[bytes_to_allocate];
                if (lutcompiler_compile(input_file, rt))
                {
                    return rt;
                }
            }
            return null;
        }
        public static byte[]? Compile(byte[] ascii_text)
        {
            int bytes_to_allocate = lutcompiler_get_output_size_raw(ascii_text, (uint)ascii_text.Length);
            if (bytes_to_allocate != -1)
            {
                byte[] rt = new byte[bytes_to_allocate];
                if (lutcompiler_compile_raw(ascii_text, (uint)ascii_text.Length, rt))
                {
                    return rt;
                }
            }
            return null;
        }

        public static bool CompileToFile(string input_file, string output_file)
        {
            return lutcompiler_compile_to_file(input_file, output_file);
        }
        public static bool CompileToFile(byte[] ascii_text, string output_file)
        {
            return lutcompiler_compile_to_file_raw(ascii_text, (uint)ascii_text.Length, output_file);
        }
    }
}
