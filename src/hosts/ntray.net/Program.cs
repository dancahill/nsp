//https://alanbondo.wordpress.com/2008/06/22/creating-a-system-tray-app-with-c/
//https://social.msdn.microsoft.com/Forums/vstudio/en-US/0913ae1a-7efc-4d7f-a7f7-58f112c69f66/c-application-system-tray-icon?forum=csharpgeneral

using System;
using System.Drawing;
using System.Windows.Forms;

namespace NTray_NET
{
	public class SysTrayApp : Form
	{
		[STAThread]
		public static void Main()
		{
			Application.Run(new SysTrayApp());
		}

		private NotifyIcon trayIcon;
		private ContextMenu trayMenu;

		public SysTrayApp()
		{
			try
			{
				trayMenu = new ContextMenu();
				trayMenu.MenuItems.Add("Test Script", TestScript);
				trayMenu.MenuItems.Add("Exit", OnExit);
				trayIcon = new NotifyIcon();
				trayIcon.Text = "NTray.NET\nPowered by wishful thinking";
				trayIcon.Icon = new Icon(System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("NTray.NET.NTray.NET.ico"));
				trayIcon.ContextMenu = trayMenu;
				trayIcon.Visible = true;
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message);
				Application.Exit();
				this.Close();
			}
		}

		private void TestScript(object sender, EventArgs e)
		{
			//throw new NotImplementedException();
			CSScripter c = new CSScripter();
			c.Test();
		}

		protected override void OnLoad(EventArgs e)
		{
			Visible = false;
			ShowInTaskbar = false;
			base.OnLoad(e);
		}

		private void OnExit(object sender, EventArgs e)
		{
			Application.Exit();
		}

		protected override void Dispose(bool isDisposing)
		{
			if (isDisposing)
			{
				trayIcon.Dispose();
			}
			base.Dispose(isDisposing);
		}
	}
}
