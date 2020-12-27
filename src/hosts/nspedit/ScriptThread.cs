using System;
using System.Threading;
using System.Windows.Forms;

namespace NSPEdit
{
	class ScriptThread
	{
		public WriteBufferDelegate WriteBuffer;
		private NSP N = new NSP();
		private Thread thread;
		private string ScriptText;
		private string SrcFile;

		public ScriptThread(string scriptText, string srcFile)
		{
			ScriptText = scriptText;
			SrcFile = srcFile;
			/* try to use the right richTextBox2 for WriteBuffer
			* this doesn't work yet because WriteBuffer is static
			* and richTextBox2 can change mid-execution...
			*/
			//richTextBox2 = Program.MainForm.richTextBox2;
		}

		public void Run()
		{
			thread = new Thread(() =>
			{
				try
				{
					NSP.WriteBuffer = WriteBuffer;
					N.ExecScript(ScriptText, SrcFile);
				}
				catch (Exception ex)
				{
					Program.MainForm.AppendOutput(ex.Message);
					MessageBox.Show(ex.Message, "RunScript()");
				}
			});
			thread.IsBackground = true;
			thread.SetApartmentState(ApartmentState.STA);
			thread.Start();
		}

		public bool IsAlive()
		{
			if (thread == null) return false;
			return thread.IsAlive;
		}

		[Obsolete]
		public void Suspend()
		{
			if (thread == null) return;
			thread.Suspend();
		}

		[Obsolete]
		public void Resume()
		{
			if (thread == null) return;
			thread.Resume();
		}
	}
}
