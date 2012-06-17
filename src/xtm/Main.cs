/* XChat Theme Manager
 *
 * Copyright (C) 2012 Patrick Griffs
 * Copyright (C) 2012 Berke Viktor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
/* using System.IO.Compression; */
using System.IO.Packaging;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;

namespace thememan
{
    public partial class XTM : Form
    {
        public string appdata = (Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\X-Chat 2\\");
        public string home = (Environment.GetFolderPath(Environment.SpecialFolder.Personal) + "/.xchat2/");

        public string xchatdir;
        public string themedir = "themes\\";

        OpenFileDialog importDialog;

        public XTM ()
		{
			InitializeComponent ();
            
			if (File.Exists ("portable-mode"))
				xchatdir = ("config\\");
			else if (Directory.Exists (appdata))
				xchatdir = (appdata);
			else if (Directory.Exists (home)) {
				xchatdir = (home); themedir = "themes/";
			} else
				Console.WriteLine("Install not found");

            ListThemes();

            String[] arguments = Environment.GetCommandLineArgs();
            if (arguments.Length > 1)
            {
                FileInfo fi = new FileInfo(arguments[1]);
                attemptImport(fi);
            }
        }

        private void ListThemes()
        {
            themelist.Items.Clear();

            if (Directory.Exists(xchatdir + themedir))
            {
                foreach (string theme in Directory.GetDirectories(xchatdir + themedir))
                {
                    themelist.Items.Add(theme.Remove(0, xchatdir.Length + themedir.Length));
                }
            }
            else
            {
                Directory.CreateDirectory(xchatdir + themedir);
            }

            if (themelist.Items.Count == 0)
            {
                applybutton.Enabled = false;
                deleteButton.Enabled = false;
            }
            else
            {
                themelist.SetSelected(0, true);
            }
        }

        private void ShowColors(List<List<string>> themecolors)
        {
            List<Control> labels = this.Controls.OfType<Label>().Cast<Control>().OrderBy(label => label.Name).ToList();
            for (byte themecolor = 0; themecolor < themecolors.Count; themecolor++)
            {
                byte rval = Convert.ToByte(int.Parse(themecolors[themecolor][0].ToString(), System.Globalization.NumberStyles.HexNumber) / 257);
                byte gval = Convert.ToByte(int.Parse(themecolors[themecolor][1].ToString(), System.Globalization.NumberStyles.HexNumber) / 257);
                byte bval = Convert.ToByte(int.Parse(themecolors[themecolor][2].ToString(), System.Globalization.NumberStyles.HexNumber) / 257);
                
                if (themecolor <= 15)
                    labels[themecolor].BackColor = Color.FromArgb(rval, gval, bval);
                else if (themecolor == 16)
                    themecolorfgmarked.ForeColor = Color.FromArgb(rval, gval, bval);
                else if (themecolor == 17)
                    themecolorfgmarked.BackColor = Color.FromArgb(rval, gval, bval);
                else if (themecolor == 18)
                    themecolorfg.ForeColor = Color.FromArgb(rval, gval, bval);
                else if (themecolor == 19)
                {
                    themecolortextbg.BackColor = Color.FromArgb(rval, gval, bval);
                    themecolorfg.BackColor = themecolortextbg.BackColor;
                }
            }
        }

        private List<List<string>> ReadTheme(string theme)
        {
            List<List<string>> themecolors = new List<List<string>>();
            foreach (string line in File.ReadLines(xchatdir + themedir + theme + "/colors.conf"))
            {
                List<string> colors = new List<string>();
                List<string> colorlist = new List<string>();
                string[] possiblecolors = { "color_256", "color_257", "color_258", "color_259" };

                for (byte num = 16; num <=31; num++)
                    colorlist.Add("color_" + num);
                colorlist.AddRange(possiblecolors);

                string[] config = line.Split(new char[] { ' ' });
                if(colorlist.Contains(config[0]) == true)
                {
                    colors.Add(config[2]);
                    colors.Add(config[3]);
                    colors.Add(config[4]);
                    themecolors.Add(colors);
                }
            }
            return themecolors;
        }

        private void applybutton_Click_1(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("XChat must be closed and this will overwrite your current theme!\n\nDo you wish to continue?", "Warning", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning);
            if (result == DialogResult.OK)
            {
                File.Copy(xchatdir + themedir + themelist.SelectedItem.ToString() + "\\colors.conf", xchatdir + "colors.conf", true);
                if (File.Exists(xchatdir + themedir + themelist.SelectedItem.ToString() + "\\pevents.conf"))
                {
                    File.Copy(xchatdir + themedir + themelist.SelectedItem.ToString() + "\\pevents.conf", xchatdir + "pevents.conf", true);
                }
            }
        }

        private void theme_selected(object sender, EventArgs e)
        {
            if (themelist.SelectedItem != null)
            {
                ShowColors(ReadTheme(themelist.SelectedItem.ToString()));
                applybutton.Enabled = true;
                deleteButton.Enabled = true;
            }
        }

        private void importbutton_Click_1(object sender, EventArgs e)
        {
            importDialog = new OpenFileDialog();
            importDialog.Filter = "XChat Theme Files|*.xct";
            importDialog.FilterIndex = 1;
            importDialog.FileOk += new CancelEventHandler(importdialog_FileOk);
            importDialog.ShowDialog();
        }

        private void importdialog_FileOk(object sender, System.ComponentModel.CancelEventArgs e)
        {
            FileInfo fi = new FileInfo(importDialog.FileName);
            attemptImport(fi);
        }

        private void attemptImport(FileInfo fi)
        {
            string themeName = fi.Name.Remove(fi.Name.Length - fi.Extension.Length);
            int result = extractTheme(fi);
            ListThemes();
            /* although a check is added to ListThemes(), this would still fail if the theme file was invalid or the theme is already installed */
            switch (result)
            {
                case 0:
                    themelist.SetSelected(themelist.FindStringExact(themeName), true);
                    /* required for command line invoking */
                    ShowColors(ReadTheme(themeName));
                    break;
                case 1:
                    MessageBox.Show("This theme is already installed!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    themelist.SetSelected(themelist.FindStringExact(themeName), true);
                    /* required for command line invoking */
                    ShowColors(ReadTheme(themeName));
                    break;
                case 2:
                    MessageBox.Show("Invalid theme file!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;
            }
        }
        /* gzip solution, not good enough coz we need multiple files
         * 
        public string extractTheme(FileInfo fi)
        {
            using (FileStream inFile = fi.OpenRead())
            {
                string themeName = fi.Name.Remove(fi.Name.Length - fi.Extension.Length);
                string destFolder = xchatdir + themedir + themeName;

                if (!Directory.Exists(destFolder))
                {
                    Directory.CreateDirectory(destFolder);
                }

                using (FileStream outFile = File.Create(destFolder + "\\colors.conf"))
                {
                    using (GZipStream Decompress = new GZipStream(inFile, CompressionMode.Decompress))
                    {
                        Decompress.CopyTo(outFile);
                    }
                }
                return themeName;
            }
        }
         */

        /* using System.IO.Package:
         * http://weblogs.asp.net/jgalloway/archive/2007/10/25/creating-zip-archives-in-net-without-an-external-library-like-sharpziplib.aspx
         * [Content_Types].xml must be present for every zip file
         */

        private const long BUFFER_SIZE = 4096;

        private int extractTheme(FileInfo zipFile)
        {
            string themeName = zipFile.Name.Remove(zipFile.Name.Length - zipFile.Extension.Length);
            string destFolder = xchatdir + themedir + themeName;

            try
            {
                using (Package zip = Package.Open(zipFile.FullName, FileMode.Open))
                {
                    PackagePartCollection parts = zip.GetParts();

                    if (Directory.Exists(destFolder))
                    {
                        return 1;
                    }
                    else
                    {
                        Directory.CreateDirectory(destFolder);
                    }

                    foreach (PackagePart part in parts)
                    {
                        /* not sure what's this good for */
                        /* String archive = part.Uri.ToString().Replace(@"/", @"\"); */
                        String destFile = destFolder + part.Uri.ToString();

                            using (FileStream outFileStream = new FileStream(destFile, FileMode.CreateNew, FileAccess.ReadWrite))
                            {
                                using (Stream inStream = part.GetStream())
                                {
                                    long bufferSize = inStream.Length < BUFFER_SIZE ? inStream.Length : BUFFER_SIZE;
                                    byte[] buffer = new byte[bufferSize];
                                    int bytesRead = 0;
                                    long bytesWritten = 0;

                                    while ((bytesRead = inStream.Read(buffer, 0, buffer.Length)) != 0)
                                    {
                                        outFileStream.Write(buffer, 0, bytesRead);
                                        bytesWritten += bufferSize;
                                    }
                                }
                            }


                    }
                }
            }
            catch (System.IO.FileFormatException)
            {
                return 2;
            }

            if (IsDirectoryEmpty(destFolder))
            {
                Directory.Delete(destFolder);
                return 2;
            }
            else
            {
                return 0;
            }
        }

        public bool IsDirectoryEmpty(string path)
        {
            return !Directory.EnumerateFileSystemEntries(path).Any();
        }

        private void deleteButton_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you want to delete this theme from the theme repo?\n\nYour currently applied theme won't be affected.", "Warning", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning);
            if (result == DialogResult.OK)
            {
                Directory.Delete(xchatdir + themedir + themelist.SelectedItem.ToString(), true);
                ListThemes();
                if (themelist.Items.Count == 0)
                {
                    deleteButton.Enabled = false;
                }
            }
        }



    }
}
