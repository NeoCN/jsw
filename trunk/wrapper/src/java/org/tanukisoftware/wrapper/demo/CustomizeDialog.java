package org.tanukisoftware.wrapper.demo;

import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSeparator;
import javax.swing.JTextField;
import javax.swing.filechooser.FileFilter;
import org.tanukisoftware.wrapper.demo.ExtensionFilter;

;
class CustomizeDialog extends JDialog
{
    /**
     * 
     */
    private static final long serialVersionUID = -5258541969990785046L;
    private JPanel jPanel1;
    private JLabel jLabel1, jLabel2, jLabel3;
    private JButton jButton1, jButton2, jButton3, jButton4, jButton5;
    private JTextField jTextField1, jTextField2, jTextField3;
    private JSeparator jSeparator1;

    private String selectedSource, selectedIcon, selectedSplashScreen, selectedDestination;
    private int result;

    public int getResult()
    {
        return result;
    }

    public String getSelectedSource()
    {
        return selectedSource;
    }

    public String getSelectedIcon()
    {
        return selectedIcon;
    }

    public String getSelectedSplashScreen()
    {
        return selectedSplashScreen;
    }

    public String getSelectedDestination()
    {
        return selectedDestination;
    }

    protected CustomizeDialog()
    {
        super();
        java.awt.GridBagConstraints gridBagConstraints;

        jPanel1 = new javax.swing.JPanel();
        jLabel1 = new javax.swing.JLabel();
        jLabel2 = new javax.swing.JLabel();
        jLabel3 = new javax.swing.JLabel();
        jButton1 = new javax.swing.JButton();
        jButton2 = new javax.swing.JButton();
        jButton3 = new javax.swing.JButton();
        jButton4 = new javax.swing.JButton();
        jButton5 = new javax.swing.JButton();
        jTextField1 = new javax.swing.JTextField();
        jTextField2 = new javax.swing.JTextField();
        jTextField3 = new javax.swing.JTextField();
        jSeparator1 = new javax.swing.JSeparator();

        setDefaultCloseOperation( javax.swing.WindowConstants.HIDE_ON_CLOSE );
        this.getContentPane().setLayout( new java.awt.GridBagLayout() );
        this.setTitle( "Wrapper Demo App: Customize" );
        jPanel1.setLayout( new java.awt.GridBagLayout() );

        jLabel1.setHorizontalAlignment( javax.swing.SwingConstants.RIGHT );
        jLabel1.setText( "Target" );
        jPanel1.add( jLabel1, new java.awt.GridBagConstraints() );

        jLabel2.setHorizontalAlignment( javax.swing.SwingConstants.RIGHT );
        jLabel2.setText( "Icon" );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        jPanel1.add( jLabel2, gridBagConstraints );

        jLabel3.setText( "Splashscreen" );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        jPanel1.add( jLabel3, gridBagConstraints );

        jButton1.setText( "..." );

        jButton1.addActionListener( new ActionListener()
        {
            public void actionPerformed( ActionEvent e )
            {
                JFileChooser fd = new JFileChooser();
                fd.setMultiSelectionEnabled( false );
                fd.setCurrentDirectory( new File( "." ) );
                fd.setDialogTitle( "Select Executable for cutomization" );
                fd.setFileHidingEnabled( true );
                fd.setApproveButtonText( "Load Executable" );
                FileFilter filter = new ExtensionFilter( "Wrapper Executable (*.exe)", new String[] { "exe" } );
                fd.setFileFilter( filter );
                int returnVal = fd.showOpenDialog( CustomizeDialog.this );

                if ( returnVal == JFileChooser.APPROVE_OPTION )
                {

                    try
                    {
                        selectedSource = fd.getSelectedFile().getCanonicalPath();
                        jTextField1.setText( selectedSource );
                        // System.out.println( "opening " + selectedSource );
                    }
                    catch ( IOException e1 )
                    {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }

                }
            }
        } );

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        jPanel1.add( jButton1, gridBagConstraints );

        jButton2.setText( "..." );
        jButton2.addActionListener( new ActionListener()
        {
            public void actionPerformed( ActionEvent e )
            {
                JFileChooser fd = new JFileChooser();
                fd.setMultiSelectionEnabled( false );
                fd.setCurrentDirectory( new File( "." ) );
                fd.setDialogTitle( "Select Icon for cutomization" );
                fd.setFileHidingEnabled( true );
                fd.setApproveButtonText( "Load Icon" );
                FileFilter filter = new ExtensionFilter( "Icon File (*.ico)", new String[] { "ico" } );
                fd.setFileFilter( filter );
                int returnVal = fd.showOpenDialog( CustomizeDialog.this );

                if ( returnVal == JFileChooser.APPROVE_OPTION )
                {

                    try
                    {
                        selectedIcon = fd.getSelectedFile().getCanonicalPath();
                        jTextField2.setText( selectedIcon );
                        // System.out.println( "opening " + selectedIcon );

                    }
                    catch ( IOException e1 )
                    {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }

                }
            }
        } );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        jPanel1.add( jButton2, gridBagConstraints );

        jButton3.setText( "..." );
        jButton3.addActionListener( new ActionListener()
        {
            public void actionPerformed( ActionEvent e )
            {
                JFileChooser fd = new JFileChooser();
                fd.setMultiSelectionEnabled( false );
                fd.setCurrentDirectory( new File( "." ) );
                fd.setDialogTitle( "Select SplashScreen for cutomization" );
                fd.setFileHidingEnabled( true );
                fd.setApproveButtonText( "Load SplashScreen" );
                FileFilter filter = new ExtensionFilter( "SplashScreen File (*.bmp)", new String[] { "bmp" } );
                fd.setFileFilter( filter );
                int returnVal = fd.showOpenDialog( CustomizeDialog.this );

                if ( returnVal == JFileChooser.APPROVE_OPTION )
                {

                    try
                    {
                        selectedSplashScreen = fd.getSelectedFile().getCanonicalPath();
                        jTextField3.setText( selectedSplashScreen );
                        // System.out.println( "opening " + selectedIcon );
                    }
                    catch ( IOException e1 )
                    {
                        // TODO Auto-generated catch block
                        e1.printStackTrace();
                    }

                }
            }
        } );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        jPanel1.add( jButton3, gridBagConstraints );

        jButton4.setText( "Customize" );
        jButton4.addActionListener( new ActionListener()
        {
            public void actionPerformed( ActionEvent e )
            {
                String errorMsg = "";
                if ( jTextField1.getText().length() == 0 || !new File( jTextField1.getText() ).exists() )
                {
                    jTextField1.setBackground( Color.red );
                    errorMsg = errorMsg.concat( "No valid Target Specified\n" );
                }

                if ( jTextField2.getText().length() != 0 && !new File( jTextField2.getText() ).exists() )
                {
                    jTextField2.setBackground( Color.red );
                    errorMsg = errorMsg.concat( "No valid Icon File Specified\n" );
                }
                if ( jTextField3.getText().length() != 0 && !new File( jTextField3.getText() ).exists() )
                {
                    jTextField3.setBackground( Color.red );
                    errorMsg = errorMsg.concat( "No valid SplashScreen File Specified\n" );
                }
                if ( jTextField2.getText().length() == 0 && jTextField3.getText().length() == 0 )
                {
                    errorMsg = errorMsg.concat( "Please input at least a Icon or SplashScreen File.\n" );
                    jTextField2.setBackground( Color.yellow );
                    jTextField3.setBackground( Color.yellow );
                }

                if ( errorMsg.length() > 0 )
                {
                    JOptionPane.showMessageDialog( CustomizeDialog.this, "See the following list of errors:\n" + errorMsg, "Error", JOptionPane.ERROR_MESSAGE );
                    return;
                }

                JFileChooser fd = new JFileChooser();
                fd.setMultiSelectionEnabled( false );

                fd.setCurrentDirectory( new File( "." ) );
                fd.setDialogTitle( "Select destination file of the cutomization" );
                fd.setFileHidingEnabled( true );

                fd.setApproveButtonText( "Custimize app" );
                FileFilter filter = new ExtensionFilter( "Executable File (*.exe)", new String[] { "exe" } );
                fd.setFileFilter( filter );

                int returnVal = fd.showSaveDialog( CustomizeDialog.this );

                if ( returnVal == JFileChooser.APPROVE_OPTION )
                {
                    try
                    {
                        selectedDestination = fd.getSelectedFile().getCanonicalPath();
                        if ( !selectedDestination.toLowerCase().endsWith( ".exe" ) )
                        {
                            selectedDestination = selectedDestination.concat( ".exe" );
                        }

                        if ( selectedDestination.equals( jTextField1.getText() ) )
                        {
                            JOptionPane.showMessageDialog( CustomizeDialog.this, "You cannot set the Source and Destination to the same File", "Error", JOptionPane.ERROR_MESSAGE );
                            selectedDestination = "";
                        }
                        else
                        {
                            result = 1;
                            CustomizeDialog.this.setVisible( false );

                        }
                    }
                    catch ( IOException ex )
                    {
                        ex.printStackTrace();
                    }

                }
            }
        } );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        jPanel1.add( jButton4, gridBagConstraints );

        jButton5.setText( "Cancel" );
        jButton5.addActionListener( new ActionListener()
        {

            public void actionPerformed( ActionEvent e )
            {
                result = 0;
                CustomizeDialog.this.setVisible( false );
            }
        } );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        jPanel1.add( jButton5, gridBagConstraints );

        jTextField1.setColumns( 20 );
        jTextField1.setToolTipText( "" );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        jPanel1.add( jTextField1, gridBagConstraints );

        jTextField2.setColumns( 20 );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        jPanel1.add( jTextField2, gridBagConstraints );

        jTextField3.setColumns( 20 );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        jPanel1.add( jTextField3, gridBagConstraints );
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        jPanel1.add( jSeparator1, gridBagConstraints );

        this.getContentPane().add( jPanel1, new java.awt.GridBagConstraints() );
        this.setLocation( this.getParent().getLocation() );
        this.setResizable( false );
        this.setModal( true );
        this.pack();

        pack();
    }

}
