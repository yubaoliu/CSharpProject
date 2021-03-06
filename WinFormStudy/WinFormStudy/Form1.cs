﻿using System;
using System.Linq;
using System.Windows.Forms;

namespace WinFormStudy
{
  public partial class Form1 : Form
  {
    public Form1()
    {
      InitializeComponent();
    }

    private void button_showName_Click(object sender, EventArgs e)
    {
   
      if (MessageBox.Show(textBox_name.Text, "Your name", MessageBoxButtons.YesNoCancel) == DialogResult.Yes)
      {
        Console.WriteLine("Your name is:\t" + textBox_name.Text);
      }
    }

    private void textBox_name_TextChanged(object sender, EventArgs e)
    {
      label_count.Text = textBox_name.Text.Count().ToString();
    }

    private void button1_Click(object sender, EventArgs e)
    {
      listBox_name.Items.Add(textBox_name.Text);
      

    }

    private void button_clear_Click(object sender, EventArgs e)
    {
      listBox_name.Items.Clear();
    }

    private void button_nextForm_Click(object sender, EventArgs e)
    {
      Form2 form2 = new Form2(); //equals to var form2=new Form2()

      //form2.Show();
       form2.ShowDialog();
      //form2.Hide();
    }

    private void Form1_Load(object sender, EventArgs e)
    {
      button_showName.Text = "OK";
    }

    private void fileToolStripMenuItem_Click(object sender, EventArgs e)
    {

    }

    private void closeToolStripMenuItem_Click(object sender, EventArgs e)
    {
      this.Close();
    }
    int count = 0;
    private void menuStrip_file_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
    {
      if(e.ClickedItem == closeToolStripMenuItem)
      { 
        count++;
      }
    }

    private void Form1_FormClosed(object sender, FormClosedEventArgs e)
    {
      if(e.CloseReason == CloseReason.UserClosing)
      {
     //   e.Cancel = true;
        this.Hide();
      }
    }
  }
}
