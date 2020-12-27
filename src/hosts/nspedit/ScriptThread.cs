using System;
using System.Threading;
using System.Threading.Tasks;
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
					NSP.SuspendScript = Suspend;
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

		public void Suspend()
		{
			if (thread == null) return;
			new Task(() => MessageBox.Show("debug.break() called\r\nPress F5 to continue", "Script Breakpoint")).Start();
#pragma warning disable CS0618 // Type or member is obsolete
			thread.Suspend();
#pragma warning restore CS0618 // Type or member is obsolete

		}

		public void Resume()
		{
			if (thread == null) return;
#pragma warning disable CS0618 // Type or member is obsolete
			thread.Resume();
#pragma warning restore CS0618 // Type or member is obsolete
		}
	}
}
