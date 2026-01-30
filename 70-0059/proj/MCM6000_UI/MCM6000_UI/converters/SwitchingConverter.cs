using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows;

namespace MCM6000_UI.converters
{
    // Converts one object to another based on compiled values.
    [ValueConversion(typeof(bool), typeof(object))]
    public class SwitchingConverter : DependencyObject, IValueConverter
    {
        public static readonly DependencyProperty TrueObjectProperty = DependencyProperty.Register(
            "TrueObject", typeof(object),
            typeof(SwitchingConverter)
        );

        public static readonly DependencyProperty FalseObjectProperty = DependencyProperty.Register(
            "FalseObject", typeof(object),
            typeof(SwitchingConverter)
        );


        public object TrueObject
        {
            get => GetValue(TrueObjectProperty);
            set => SetValue(TrueObjectProperty, value);
        }

        public object FalseObject
        {
            get => GetValue(FalseObjectProperty);
            set => SetValue(FalseObjectProperty, value);
        }


        #region IValueConverter Members

        public object Convert(object value, Type targetType, object parameter,
            CultureInfo culture)
        {
            return (bool)value ? TrueObject : FalseObject;
        }

        public object ConvertBack(object value, Type targetType, object parameter,
            CultureInfo culture)
        {
            if ((TrueObject is null && FalseObject is null) || TrueObject.Equals(FalseObject))
                throw new NotSupportedException("True and false objects are identical");

            if ((value is null && TrueObject is null) || value.Equals(TrueObject))
                return true;
            if ((value is null && FalseObject is null) || value.Equals(FalseObject))
                return false;

            throw new ArgumentException("The value provided was not one of the switching objects");
        }

        #endregion


        public SwitchingConverter()
        {
            TrueObject = null;
            FalseObject = null;
        }
    }
}
