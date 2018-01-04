using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
//using Microsoft.CSharp.;

namespace NTray_NET
{
	public class CSScripter
	{
		public CSScripter()
		{
		}

		public static bool testbool = false;

		public void Test()
		{
			try
			{
				//string src = "using System.Windows.Forms;\r\nclass Test\r\n{\r\nstatic void Main(string[] args)\r\n{\r\nMessageBox.Show(\"Hello World!\", \"NScript\");\r\n}\r\n};";

				AppDomain domain = AppDomain.CreateDomain("MyDomain");
				Console.WriteLine("Host domain: " + AppDomain.CurrentDomain.FriendlyName);
				Console.WriteLine("child domain: " + domain.FriendlyName);

				CompilerParameters compilerparams = new CompilerParameters();
				compilerparams.ReferencedAssemblies.Add(Application.ExecutablePath);
				compilerparams.ReferencedAssemblies.Add("System.Windows.Forms.dll");
				compilerparams.GenerateInMemory = true;
				compilerparams.GenerateExecutable = true;

				CodeDomProvider provider = new Microsoft.CSharp.CSharpCodeProvider();
				CompilerResults results = provider.CompileAssemblyFromFile(compilerparams, System.IO.Path.Combine(Application.StartupPath, "NTray.NET.cs"));
				//System.CodeDom.Compiler.CompilerResults results = provider.CompileAssemblyFromSource(compilerparams, src);

				if (results.Errors.HasErrors)
				{
					string s = "";
					for (int i = 0; i < 10 && i < results.Errors.Count; i++)
					{
						if (i > 0) s += "\r\n";
						s += string.Format("Line {0}: {1}", results.Errors[i].Line, results.Errors[i].ErrorText);
					}
					throw new Exception(s);
				}

				results.CompiledAssembly.EntryPoint.Invoke(null, System.Reflection.BindingFlags.Static, null, new object[] { new string[] { "a", "b" } }, null);
				//results.CompiledAssembly.EntryPoint.Invoke(null, System.Reflection.BindingFlags.Instance, null, new object[] { args }, null);

				Assembly generatedAssembly = results.CompiledAssembly;
				//generatedAssembly.CreateInstance("Test");
				Type type = generatedAssembly.GetType("Test");
				object obj = Activator.CreateInstance(type);

				//var obj = Activator.CreateInstance(type);
				//type.InvokeMember("Main2", BindingFlags.Default | BindingFlags.InvokeMethod, null, obj, null);
				//type.InvokeMember("Main2", BindingFlags.Default | BindingFlags.InvokeMethod, null, obj, new string[] { });

				//method.Invoke(obj, null);
				type.GetMethod("Main2").Invoke(obj, new object[] { new string[] { "a", "b" } });
				type.GetMethod("Main3").Invoke(obj, new object[] { });
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "assembly exception", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			Console.WriteLine("NTray_NET.CSScripter.testbool: " + NTray_NET.CSScripter.testbool);
		}
	}
}
