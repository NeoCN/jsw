package org.tanukisoftware.wrapper.demo;

import java.awt.Color;
import java.awt.Cursor;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.IOException;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;

public class AboutDialog extends JDialog
{
    private static final long serialVersionUID = 1L;

    public AboutDialog( JFrame parent )
    {
        super( parent, "About Dialog", true );
        this.setResizable( false );
        Box b = Box.createVerticalBox();
        b.add( Box.createGlue() );
        b.add( new JLabel( "Demo Application for the Java Service Wrapper" ) );
        b.add( new JLabel( "By Tanuki Software Ltd." ) );
        final JLabel url = new JLabel();
        url.setText( "<html><u>http://wrapper.tanukisoftware.org</u></html>" );
        url.setForeground( Color.BLUE );
        url.addMouseListener( new MouseListener()
        {

            public void mouseReleased( MouseEvent e )
            {
                // TODO Auto-generated method stub
            }

            public void mousePressed( MouseEvent e )
            {
                // TODO Auto-generated method stub
            }

            public void mouseExited( MouseEvent e )
            {
                url.setCursor( new Cursor( Cursor.DEFAULT_CURSOR ) );
            }

            public void mouseEntered( MouseEvent e )
            {
                url.setCursor( new Cursor( Cursor.HAND_CURSOR ) );
            }

            public void mouseClicked( MouseEvent e )
            {

                if ( e.getClickCount() > 0 )
                {
                    try
                    {
                        Runtime.getRuntime().exec( "cmd.exe /c start http://wrapper.tanukisoftware.org" );
                    }
                    catch ( IOException ex )
                    {
                        try
                        {
                            Runtime.getRuntime().exec( "firefox http://wrapper.tanukisoftware.org" );
                        }
                        catch ( IOException e1 )
                        {

                            System.out.println( ex.getMessage() );
                            System.out.println();
                        }
                    }
                }

            }
        } );

        b.add( url );
        b.add( Box.createGlue() );
        getContentPane().add( b, "Center" );

        JPanel p2 = new JPanel();
        JButton ok = new JButton( "Ok " );
        p2.add( ok );
        getContentPane().add( p2, "South" );

        ok.addActionListener( new ActionListener()
        {
            public void actionPerformed( ActionEvent evt )
            {
                setVisible( false );
            }
        } );
        this.setLocation( this.getParent().getLocation() );
        this.pack();// setSize(250, 150);
    }

}