using NZBDrive.Model;
using System;
using System.Collections.Generic;
using System.Globalization;
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
    public partial class NZBFilePartListView : UserControl
    {
        public NZBFilePartListView()
        {
            InitializeComponent();
        }
    }

    public class NZBFilePartColorConverter : IValueConverter
    {
        private SolidColorBrush NormalColor = new SolidColorBrush(Colors.White);
        private SolidColorBrush ErrorColor = new SolidColorBrush(Colors.Red);

        public object Convert(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            return (int)value == 0 ? NormalColor : ErrorColor;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
