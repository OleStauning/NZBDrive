﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace NZBDrive.View
{

    [ProvideToolboxControl("General", true)]
    public partial class OkCancelControl : UserControl
    {
//        private RoutedEventHandler _clickOK;

        public RoutedEventHandler ClickOK
        {
            get { return (RoutedEventHandler)this.GetValue(ClickOKProperty); }
            set { this.SetValue(ClickOKProperty, value); }
        }
        public static readonly DependencyProperty ClickOKProperty = DependencyProperty.Register(
          "ClickOK", typeof(RoutedEventHandler), typeof(OkCancelControl), new PropertyMetadata(null));

        public OkCancelControl()
        {
            InitializeComponent();
        }

        private void OKButton_Click(object sender, RoutedEventArgs e)
        {
            var h = ClickOK;
            if (h != null) h(sender, e);
        }
    }
}
